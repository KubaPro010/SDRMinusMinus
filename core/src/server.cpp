#include "server.h"
#include "core.h"
#include "utils/flog.h"
#include "version.h"
#include "config.h"
#include <filesystem>
#include "dsp/types.h"
#include "source.h"
#include "gui/smgui.h"
#include "utils/optionlist.h"
#include "dsp/compression/sample_stream_compressor.h"
#include "dsp/sink/handler_sink.h"
#include <zstd.h>

#define LoadedModuleName "MiriSDR"
#define ModuleName "mirisdr_source"

#define SourceName "MiriSDR"

namespace server {
    dsp::stream<dsp::complex_t> dummyInput;
    dsp::compression::SampleStreamCompressor comp;
    dsp::sink::Handler<uint8_t> hnd;
    net::Conn client;
    uint8_t* rbuf = NULL; // Receive buffer
    uint8_t* sbuf = NULL; // General send buffer
    uint8_t* bbuf = NULL; // Used for sending the baseband data

    PacketHeader* r_pkt_hdr = NULL;
    uint8_t* r_pkt_data = NULL;
    CommandHeader* r_cmd_hdr = NULL;
    uint8_t* r_cmd_data = NULL;

    PacketHeader* s_pkt_hdr = NULL;
    uint8_t* s_pkt_data = NULL;
    CommandHeader* s_cmd_hdr = NULL;
    uint8_t* s_cmd_data = NULL;

    PacketHeader* bb_pkt_hdr = NULL;
    uint8_t* bb_pkt_data = NULL;

    SmGui::DrawListElem dummyElem;

    ZSTD_CCtx* cctx;

    net::Listener listener;

    int sourceId = 0;
    bool running = false;
    bool compression = false;
    double sampleRate = 1000000.0;

    int main() {
        flog::info("=====| SERVER MODE |=====");

        // Init DSP
        comp.init(&dummyInput, dsp::compression::PCM_TYPE_F32);
        hnd.init(&comp.out, _testServerHandler, NULL);
        rbuf = new uint8_t[SERVER_MAX_PACKET_SIZE];
        sbuf = new uint8_t[SERVER_MAX_PACKET_SIZE];
        bbuf = new uint8_t[SERVER_MAX_PACKET_SIZE];
        comp.start();
        hnd.start();

        // Initialize headers
        r_pkt_hdr = (PacketHeader*)rbuf;
        r_pkt_data = &rbuf[sizeof(PacketHeader)];
        r_cmd_hdr = (CommandHeader*)r_pkt_data;
        r_cmd_data = &rbuf[sizeof(PacketHeader) + sizeof(CommandHeader)];

        s_pkt_hdr = (PacketHeader*)sbuf;
        s_pkt_data = &sbuf[sizeof(PacketHeader)];
        s_cmd_hdr = (CommandHeader*)s_pkt_data;
        s_cmd_data = &sbuf[sizeof(PacketHeader) + sizeof(CommandHeader)];

        bb_pkt_hdr = (PacketHeader*)bbuf;
        bb_pkt_data = &bbuf[sizeof(PacketHeader)];

        // Initialize compressor
        cctx = ZSTD_createCCtx();

        // Load config
        core::configManager.acquire();
        std::string modulesDir = core::configManager.conf["modulesDirectory"];
        core::configManager.release();
        modulesDir = std::filesystem::absolute(modulesDir).string();

        if (std::filesystem::is_directory(modulesDir)) {
            for (const auto& file : std::filesystem::directory_iterator(modulesDir)) {
                std::string path = file.path().generic_string();
                std::string fn = file.path().filename().string();
                if (file.path().extension().generic_string() != SDRMM_MOD_EXTENTSION) continue;
                if (!file.is_regular_file()) continue;
                if (fn.find("source") == std::string::npos) continue;

                flog::info("Loading {0}", path);
                core::moduleManager.loadModule(path);
            }
        }
        else flog::warn("Module directory {0} does not exist, not loading modules from directory", modulesDir);
	    core::moduleManager.createInstance(LoadedModuleName, ModuleName);

        // Do post-init
        core::moduleManager.doPostInitAll();

        core::sourceManager.selectSource(SourceName);

        std::string host = (std::string)core::args["addr"];
        int port = (int)core::args["port"];
        listener = net::listen(host, port);
        listener->acceptAsync(_clientHandler, NULL);

        flog::info("Ready, listening on {0}:{1}", host, port);
        while(1) std::this_thread::sleep_for(std::chrono::milliseconds(100)); 

        return 0;
    }

    void _clientHandler(net::Conn conn, void* ctx) {
        // Reject if someone else is already connected
        if (client && client->isOpen()) {
            char client_ip[16];
            inet_ntop(AF_INET, &conn->remoteAddr.sin_addr, client_ip, sizeof(client_ip));
            char port_str[6];
            snprintf(port_str, sizeof(port_str), "%u", ntohs(conn->remoteAddr.sin_port));
            flog::info("REJECTED Connection from {0}:{1}, another client is already connected.", client_ip, port_str);
            
            // Issue a disconnect command to the client
            uint8_t buf[sizeof(PacketHeader) + sizeof(CommandHeader)];
            PacketHeader* tmp_phdr = (PacketHeader*)buf;
            CommandHeader* tmp_chdr = (CommandHeader*)&buf[sizeof(PacketHeader)];
            tmp_phdr->size = sizeof(PacketHeader) + sizeof(CommandHeader);
            tmp_phdr->type = PACKET_TYPE_COMMAND;
            tmp_chdr->cmd = COMMAND_DISCONNECT;
            conn->write(tmp_phdr->size, buf);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            conn->close();
            
            // Start another async accept
            listener->acceptAsync(_clientHandler, NULL);
            return;
        }

        char client_ip[16];
        inet_ntop(AF_INET, &conn->remoteAddr.sin_addr, client_ip, sizeof(client_ip));
        char port_str[6];
        snprintf(port_str, sizeof(port_str), "%u", ntohs(conn->remoteAddr.sin_port));
        
        flog::info("Connection from {0}:{1}", client_ip, port_str);
        client = std::move(conn);
        client->readAsync(sizeof(PacketHeader), rbuf, _packetHandler, NULL);

        // Perform settings reset
        core::sourceManager.stop();
        comp.setPCMType(dsp::compression::PCM_TYPE_F32);
        compression = false;

        sendSampleRate(sampleRate);

        listener->acceptAsync(_clientHandler, NULL);
    }

    void _packetHandler(int count, uint8_t* buf, void* ctx) {
        PacketHeader* hdr = (PacketHeader*)buf;

        // Read the rest of the data
        int len = 0;
        int read = 0;
        int goal = hdr->size - sizeof(PacketHeader);
        if (goal > SERVER_MAX_PACKET_SIZE - sizeof(PacketHeader)) { return; }
        auto starttime = std::chrono::steady_clock::now();
        while (len < goal) {
            if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - starttime).count() > 150) { return; }
            read = client->read(goal - len, &buf[sizeof(PacketHeader) + len]);
            if (read < 0) { return; };
            len += read;
        }

        // Parse and process
        if (hdr->type == PACKET_TYPE_COMMAND && hdr->size >= sizeof(PacketHeader) + sizeof(CommandHeader)) {
            CommandHeader* chdr = (CommandHeader*)&buf[sizeof(PacketHeader)];
            commandHandler((Command)chdr->cmd, &buf[sizeof(PacketHeader) + sizeof(CommandHeader)], hdr->size - sizeof(PacketHeader) - sizeof(CommandHeader));
        }
        else sendError(ERROR_INVALID_PACKET);

        // Start another async read
        client->readAsync(sizeof(PacketHeader), rbuf, _packetHandler, NULL);
    }

    void _testServerHandler(uint8_t* data, int count, void* ctx) {
        // Compress data if needed and fill out header fields
        if (compression) {
            bb_pkt_hdr->type = PACKET_TYPE_BASEBAND_COMPRESSED;
            bb_pkt_hdr->size = sizeof(PacketHeader) + (uint32_t)ZSTD_compressCCtx(cctx, &bbuf[sizeof(PacketHeader)], SERVER_MAX_PACKET_SIZE-sizeof(PacketHeader), data, count, 1);
        }
        else {
            bb_pkt_hdr->type = PACKET_TYPE_BASEBAND;
            bb_pkt_hdr->size = sizeof(PacketHeader) + count;
            memcpy(&bbuf[sizeof(PacketHeader)], data, count);
        }

        // Write to network
        if (client && client->isOpen()) { client->write(bb_pkt_hdr->size, bbuf); }
    }

    void setInput(dsp::stream<dsp::complex_t>* stream) {
        comp.setInput(stream);
    }

    void commandHandler(Command cmd, uint8_t* data, int len) {
        if (cmd == COMMAND_GET_UI) sendUI(COMMAND_GET_UI, "", dummyElem);
        else if (cmd == COMMAND_UI_ACTION && len >= 3) {
            // Check if sending back data is needed
            int i = 0;
            bool sendback = data[i++];
            len--;
            
            // Load id
            SmGui::DrawListElem diffId;
            int count = SmGui::DrawList::loadItem(diffId, &data[i], len);
            if (count < 0) { sendError(ERROR_INVALID_ARGUMENT); return; }
            if (diffId.type != SmGui::DRAW_LIST_ELEM_TYPE_STRING) { sendError(ERROR_INVALID_ARGUMENT); return; } 
            i += count;
            len -= count;

            // Load value
            SmGui::DrawListElem diffValue;
            count = SmGui::DrawList::loadItem(diffValue, &data[i], len);
            if (count < 0) { sendError(ERROR_INVALID_ARGUMENT); return; }
            i += count;
            len -= count;

            // Render and send back
            if (sendback) sendUI(COMMAND_UI_ACTION, diffId.str, diffValue);
            else renderUI(NULL, diffId.str, diffValue);
        }
        else if (cmd == COMMAND_START) {
            core::sourceManager.start();
            running = true;
        }
        else if (cmd == COMMAND_STOP) {
            core::sourceManager.stop();
            running = false;
        }
        else if (cmd == COMMAND_SET_FREQUENCY && len == 8) {
            core::sourceManager.tune(*(double*)data);
            sendCommandAck(COMMAND_SET_FREQUENCY, 0);
        }
        else if (cmd == COMMAND_SET_SAMPLE_TYPE && len == 1) {
            dsp::compression::PCMType type = (dsp::compression::PCMType)*(uint8_t*)data;
            comp.setPCMType(type);
        }
        else if (cmd == COMMAND_SET_COMPRESSION && len == 1) compression = *(uint8_t*)data;
        else {
            flog::error("Invalid Command: {0} (len = {1})", (int)cmd, len);
            sendError(ERROR_INVALID_COMMAND);
        }
    }

    void drawMenu() {
        SmGui::Text(SourceName);
        core::sourceManager.showSelectedMenu();
    }

    void renderUI(SmGui::DrawList* dl, std::string diffId, SmGui::DrawListElem diffValue) {
        SmGui::setDiff(diffId, diffValue);
        if (dl && !diffId.empty()) {
            drawMenu();
            SmGui::setDiff("", dummyElem);
        }
        SmGui::startRecord(dl);
        drawMenu();
        SmGui::stopRecord();
    }

    void sendUI(Command originCmd, std::string diffId, SmGui::DrawListElem diffValue) {
        // Render UI
        SmGui::DrawList dl;
        renderUI(&dl, diffId, diffValue);

        // Create response
        int size = dl.getSize();
        dl.store(s_cmd_data, size);

        // Send to network
        sendCommandAck(originCmd, size);
    }

    void sendError(Error err) {
        PacketHeader* hdr = (PacketHeader*)sbuf;
        s_pkt_data[0] = err;
        sendPacket(PACKET_TYPE_ERROR, 1);
    }

    void sendSampleRate(double sampleRate) {
        *(double*)s_cmd_data = sampleRate;
        sendCommand(COMMAND_SET_SAMPLERATE, sizeof(double));
    }

    void setInputSampleRate(double samplerate) {
        sampleRate = samplerate;
        if (!client || !client->isOpen()) { return; }
        sendSampleRate(sampleRate);
    }

    void sendPacket(PacketType type, int len) {
        s_pkt_hdr->type = type;
        s_pkt_hdr->size = sizeof(PacketHeader) + len;
        client->write(s_pkt_hdr->size, sbuf);
    }

    void sendCommand(Command cmd, int len) {
        s_cmd_hdr->cmd = cmd;
        sendPacket(PACKET_TYPE_COMMAND, sizeof(CommandHeader) + len);
    }

    void sendCommandAck(Command cmd, int len) {
        s_cmd_hdr->cmd = cmd;
        sendPacket(PACKET_TYPE_COMMAND_ACK, sizeof(CommandHeader) + len);
    }
}
