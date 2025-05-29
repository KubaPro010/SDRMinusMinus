#include <utils/flog.h>
#include <module.h>
#include <signal_path/signal_path.h>
#include <core.h>
#include <config.h>
#include <gui/smgui.h>
#include <thread>
#include <mirisdr.h>

#define FLAVOR MIRISDR_HW_DEFAULT
// #define FLAVOR MIRISDR_HW_SDRPLAY

#define CONCAT(a, b) ((std::string(a) + b).c_str())
SDRPP_MOD_INFO{
    /* Name:             */ "mirisdr_source",
    /* Description:      */ "MiriSDR source module for SDR++",
    /* Author:           */ "cropinghigh, kuba201",
    /* Version:          */ 1, 3, 1,
    /* Max instances     */ 1,
    /* Is SDR-- Compliant*/ true
};

ConfigManager config;
const char* sampleRatesTxt = "15 MHz\0"
                             "14 MHz\0"
                             "13 MHz\0"
                             "12 MHz\0"
                             "11 MHz\0"
                             "10 MHz\0"
                             "9 MHz\0"
                             "8 MHz\0"
                             "7 MHz\0"
                             "6 MHz\0"
                             "5 MHz\0"
                             "4 MHz\0"
                             "3 MHz\0"
                             "2 MHz\0"
                             "1.54 MHz\0";

const int sampleRates[] = {
    15000000,
    14000000,
    13000000,
    12000000,
    11000000,
    10000000,
    9000000,
    8000000,
    7000000,
    6000000,
    5000000,
    4000000,
    3000000,
    2000000,
    1540000,
};

class MirisdrSourceModule : public ModuleManager::Instance {
public:
    MirisdrSourceModule(std::string name) {
        this->name = name;

        sampleRate = 1540000;
        srId = 14;

        handler.ctx = this;
        handler.selectHandler = menuSelected;
        handler.deselectHandler = menuDeselected;
        handler.menuHandler = menuHandler;
        handler.startHandler = start;
        handler.stopHandler = stop;
        handler.tuneHandler = tune;
        handler.stream = &stream;

        refresh();

        config.acquire();
        std::string confSerial = config.conf["device"];
        config.release();
        selectBySerial(confSerial);

        sourceManager.registerSource("MiriSDR", &handler);
    }

    ~MirisdrSourceModule() {
        stop(this);
        sourceManager.unregisterSource("MiriSDR");
    }

    void postInit() {}

    void enable() {
        enabled = true;
    }

    void disable() {
        enabled = false;
    }

    bool isEnabled() {
        return enabled;
    }

    void refresh() {
        devList.clear();
        devIdxList.clear();
        devListTxt = "";

        int cnt = 0;
        cnt = mirisdr_get_device_count();

        for(int i = 0; i < cnt; i++) {
            char manufact[256];
            char product[256];
            char serial[256];
            if(!mirisdr_get_device_usb_strings(i, manufact, product, serial)) {
                std::string displayName = std::string(manufact) + " " + std::string(product) + " " + std::string(serial);
                std::string name = std::string(manufact) + " " + std::string(product) + " " + std::string(serial);
                if(cnt == 1) displayName = std::string(manufact) + " " + std::string(product);
                devList.push_back(name);
                devIdxList.push_back(i);
                devListTxt += displayName;
                devListTxt += '\0';
            }
        }
    }

    void selectFirst() {
        if (devList.size() != 0) {
            selectBySerial(devList[0]);
            return;
        }
        selectedSerial = "";
    }

    void selectBySerial(std::string serial) {
        if (std::find(devList.begin(), devList.end(), serial) == devList.end()) {
            selectFirst();
            return;
        }

        bool created = false;
        config.acquire();
        if (!config.conf["devices"].contains(serial)) {
            config.conf["devices"][serial]["sampleRate"] = 1540000;
            config.conf["devices"][serial]["gain"] = 0;
            config.conf["devices"][serial]["bandwidth"] = 3;
        }
        config.release(created);

        // Set default values
        srId = 14;
        sampleRate = 1540000;
        bwId = 3;

        // Load from config if available and validate
        if (config.conf["devices"][serial].contains("sampleRate")) {
            int psr = config.conf["devices"][serial]["sampleRate"];
            for (int i = 0; i < 14; i++) {
                if (sampleRates[i] == psr) {
                    sampleRate = psr;
                    srId = i;
                }
            }
        }
        if (config.conf["devices"][serial].contains("gain")) {
            gain = config.conf["devices"][serial]["gain"];
        }
        if (config.conf["devices"][serial].contains("offsetTune")) {
            offsetTune = config.conf["devices"][serial]["offsetTune"];
        } else {
            offsetTune = false;
        }
        if (config.conf["devices"][serial].contains("bandwidth")) {
            bwId = config.conf["devices"][serial]["bandwidth"];
            bwId = std::clamp<int>(bwId, 0, 7);
        }

        selectedSerial = serial;
    }

private:
    static void menuSelected(void* ctx) {
        MirisdrSourceModule* _this = (MirisdrSourceModule*)ctx;
        core::setInputSampleRate(_this->sampleRate);
        flog::info("MirisdrSourceModule '{0}': Menu Select!", _this->name);
    }

    static void menuDeselected(void* ctx) {
        MirisdrSourceModule* _this = (MirisdrSourceModule*)ctx;
        flog::info("MirisdrSourceModule '{0}': Menu Deselect!", _this->name);
    }

    static void start(void* ctx) {
        MirisdrSourceModule* _this = (MirisdrSourceModule*)ctx;
        if (_this->running) { return; }
        if (_this->selectedSerial == "") {
            flog::error("Tried to start source with empty serial");
            return;
        }

        _this->selectBySerial(_this->selectedSerial);

        int cnt = mirisdr_get_device_count();
        int id = -1;

        // find the device index from serial
        auto serialIt = std::find(_this->devList.cbegin(), _this->devList.cend(), _this->selectedSerial);
        if (serialIt != _this->devList.cend()) {
            id = _this->devIdxList[std::distance(_this->devList.cbegin(), serialIt)];
        }

        if(id == -1) {
            flog::error("Mirisdr device is not available");
            return;
        }

        int oret = mirisdr_open(&_this->openDev, id);

        if(oret) {
            flog::error("Could not open Mirisdr {0} id {1} cnt {2}", _this->selectedSerial, id, cnt);
            return;
        }
        if(mirisdr_set_hw_flavour(_this->openDev, FLAVOR)) {
            flog::error("Could not set Mirisdr hw flavour {0}", _this->selectedSerial);
            return;
        }
        if(mirisdr_set_sample_format(_this->openDev, "AUTO")) {
            flog::error("Could not set Mirisdr sample format {0}", _this->selectedSerial);
            return;
        }
        if(mirisdr_set_transfer(_this->openDev, "BULK")) {
            flog::error("Could not set Mirisdr transfer {0}", _this->selectedSerial);
            return;
        }
        if(mirisdr_set_if_freq(_this->openDev, 0)) {
            flog::error("Could not set Mirisdr if freq {0}", _this->selectedSerial);
            return;
        }
        if(mirisdr_set_sample_rate(_this->openDev, _this->sampleRate)) {
            flog::error("Could not set Mirisdr sample rate {0}", _this->selectedSerial);
            return;
        }
        if(mirisdr_set_bandwidth(_this->openDev, get_auto_bw(ctx))) {
            flog::error("Could not set Mirisdr bandwidth {0}", _this->selectedSerial);
            return;
        }
        if(mirisdr_set_center_freq(_this->openDev, _this->freq)) {
            flog::error("Could not set Mirisdr center freq {0}", _this->selectedSerial);
            return;
        }
        if(mirisdr_set_tuner_gain_mode(_this->openDev, 1)) {
            flog::error("Could not set Mirisdr gain mode {0}", _this->selectedSerial);
            return;
        }
        if(mirisdr_set_tuner_gain(_this->openDev, _this->gain)) {
            flog::error("Could not set Mirisdr gain {0}", _this->selectedSerial);
            return;
        }
        if(mirisdr_set_offset_tuning(_this->openDev, _this->offsetTune ? 1 : 0)) {
            flog::error("Could not set Mirisdr offset tuning {0}", _this->selectedSerial);
            return;
        }

        /* Reset endpoint before we start reading from it (mandatory) */
        if(mirisdr_reset_buffer(_this->openDev)) {
            flog::error("Failed to reset Mirisdr buffer {0}", _this->selectedSerial);
            return;
        }

        _this->workerThread = std::thread(mirisdr_read_async, _this->openDev, callback, _this, 0, (_this->sampleRate/50)*sizeof(int16_t));

        _this->running = true;

        flog::info("MirisdrSourceModule '{0}': Start!", _this->name);
    }

    static void stop(void* ctx) {
        MirisdrSourceModule* _this = (MirisdrSourceModule*)ctx;
        if (!_this->running) { return; }
        _this->running = false;
        if(mirisdr_cancel_async(_this->openDev)) {
            flog::error("Mirisdr async cancel failed {0}", _this->selectedSerial);
        }
        _this->stream.stopWriter();
        _this->workerThread.join();
        int err = mirisdr_close(_this->openDev);
        if (err) {
            flog::error("Could not close Mirisdr {0}", _this->selectedSerial);
        }
        _this->stream.clearWriteStop();
        flog::info("MirisdrSourceModule '{0}': Stop!", _this->name);
    }

    static void tune(double freq, void* ctx) {
        MirisdrSourceModule* _this = (MirisdrSourceModule*)ctx;
        bool fail = false;
        if (_this->running) {
            if(freq > 2e9) fail = true;
            if(!fail) {
                if(mirisdr_set_center_freq(_this->openDev, (uint32_t)freq) || mirisdr_get_center_freq(_this->openDev) != (uint32_t)freq) flog::error("Could not set Mirisdr freq {0}(selected {1}, current {2})", _this->selectedSerial, (uint32_t)freq, mirisdr_get_center_freq(_this->openDev));
            }
        }
        if(!fail) _this->freq = freq;
        flog::info("MirisdrSourceModule '{0}': Tune: {1}!", _this->name, (uint32_t)freq);
    }

    static int get_auto_bw(void* ctx) {
        MirisdrSourceModule* _this = (MirisdrSourceModule*)ctx;
        int bandwid = 0;
        int samplerate = sampleRates[_this->srId];
        switch(samplerate) {
            case 15000000:
                bandwid = 15000000;
                break;
            case 14000000:
                bandwid = 14000000;
                break;
            case 13000000:
                bandwid = 13000000;
                break;
            case 12000000:
                bandwid = 12000000;
                break;
            case 11000000:
                bandwid = 11000000;
                break;
            case 10000000:
                bandwid = 11000000;
                break;
            case 9000000:
                bandwid = 9000000;
                break;
            case 8000000:
                bandwid = 8000000;
                break;
            case 7000000:
                bandwid = 7000000;
                break;
            case 6000000:
                bandwid = 6000000;
                break;
            case 5000000:
                bandwid = 5000000;
                break;
            default:
                bandwid = 1536000;
                break;
            //minimal sample rate is 1.54, setting the bandwidht lower than 1.536 is pointless
        }
        return bandwid;
    }

    static void menuHandler(void* ctx) {
        MirisdrSourceModule* _this = (MirisdrSourceModule*)ctx;

        if (_this->running) { SmGui::BeginDisabled(); }
        SmGui::FillWidth();
        SmGui::ForceSync();
        if (SmGui::Combo(CONCAT("##_mirisdr_dev_sel_", _this->name), &_this->devId, _this->devListTxt.c_str())) {
            _this->selectBySerial(_this->devList[_this->devId]);
            config.acquire();
            config.conf["device"] = _this->selectedSerial;
            config.release(true);
        }

        SmGui::SameLine();
        SmGui::FillWidth();
        SmGui::ForceSync();
        if (SmGui::Button(CONCAT("Refresh##_mirisdr_refr_", _this->name))) {
            _this->refresh();
            _this->selectBySerial(_this->selectedSerial);
            core::setInputSampleRate(_this->sampleRate);
        }

        if (SmGui::Combo(CONCAT("##_mirisdr_sr_sel_", _this->name), &_this->srId, sampleRatesTxt)) {
            _this->sampleRate = sampleRates[_this->srId];
            core::setInputSampleRate(_this->sampleRate);
            config.acquire();
            config.conf["devices"][_this->selectedSerial]["sampleRate"] = _this->sampleRate;
            config.release(true);
        }

        if (_this->running) { SmGui::EndDisabled(); }

        SmGui::LeftLabel("Gain");
        SmGui::FillWidth();
        if (SmGui::SliderInt(CONCAT("##_mirisdr_gain_", _this->name), &_this->gain, 0, 102)) {
            if (_this->running) {
                mirisdr_set_tuner_gain(_this->openDev, _this->gain);
            }
            config.acquire();
            config.conf["devices"][_this->selectedSerial]["gain"] = (int)_this->gain;
            config.release(true);
        }

        if (SmGui::Checkbox(CONCAT("Offset Tuning##_mirisdr_offset_", _this->name), &_this->offsetTune)) {
            if (_this->running) {
                mirisdr_set_offset_tuning(_this->openDev, _this->offsetTune ? 1 : 0);
            }
            config.acquire();
            config.conf["devices"][_this->selectedSerial]["offsetTune"] = _this->offsetTune;
            config.release(true);
        }
    }

    static void callback(unsigned char *buf, uint32_t len, void *ctx) {
        MirisdrSourceModule* _this = (MirisdrSourceModule*)ctx;
        int count = (len/sizeof(int16_t)) / 2;
        int16_t* buffer = (int16_t*)buf;
        volk_16i_s32f_convert_32f((float*)_this->stream.writeBuf, buffer, 32768.0f, count * 2);
        if (!_this->stream.swap(count)) { return; }
    }

    std::string name;
    mirisdr_dev_t* openDev;
    bool enabled = true;
    std::thread workerThread;
    dsp::stream<dsp::complex_t> stream;
    int sampleRate;
    bool offsetTune;
    SourceManager::SourceHandler handler;
    bool running = false;
    double freq;
    std::string selectedSerial = "";
    int devId = 0;
    int srId = 0;
    int bwId = 16;
    int gain = 0;

    std::vector<std::string> devList;
    std::vector<int> devIdxList;
    std::string devListTxt;
};

MOD_EXPORT void _INIT_() {
    json def = json({});
    def["devices"] = json({});
    def["device"] = "";
    config.setPath(core::args["root"].s() + "/mirisdr_config.json");
    config.load(def);
    config.enableAutoSave();
}

MOD_EXPORT ModuleManager::Instance* _CREATE_INSTANCE_(std::string name) {
    return new MirisdrSourceModule(name);
}

MOD_EXPORT void _DELETE_INSTANCE_(ModuleManager::Instance* instance) {
    delete (MirisdrSourceModule*)instance;
}

MOD_EXPORT void _END_() {
    config.disableAutoSave();
    config.save();
}
