#include <server.h>
#include "imgui.h"
#include <stdio.h>
#include <version.h>
#include <utils/flog.h>
#include <config.h>
#include <core.h>
#include <filesystem>

#ifndef INSTALL_PREFIX
#define INSTALL_PREFIX "/usr"
#endif

namespace core {
    ConfigManager configManager;
    ModuleManager moduleManager;
    ModuleComManager modComManager;
    CommandArgsParser args;

    void setInputSampleRate(double samplerate) {
        // Forward this to the server
        if (args["server"].b()) { server::setInputSampleRate(samplerate); return; }
    }
};

// main
int sdrpp_main(int argc, char* argv[]) {
    flog::info("SDR-- v" VERSION_STR);
    core::args.defineAll();
    if (core::args.parse(argc, argv) < 0) { return -1; } 

    if (core::args["help"].b()) {
        core::args.showHelp();
        return 0;
    }

    bool serverMode = (bool)core::args["server"];

    std::string root = (std::string)core::args["root"];
    if (!std::filesystem::exists(root)) {
        flog::warn("Root directory {0} does not exist, creating it", root);
        if (!std::filesystem::create_directories(root)) {
            flog::error("Could not create root directory {0}", root);
            return -1;
        }
    }

    // Check that the path actually is a directory
    if (!std::filesystem::is_directory(root)) {
        flog::error("{0} is not a directory", root);
        return -1;
    }

    // ======== DEFAULT CONFIG ========
    json defConfig;
    defConfig["modulesDirectory"] = INSTALL_PREFIX "/lib/sdrpp/plugins";
    flog::info("Loading config");
    core::configManager.setPath(root + "/config.json");
    core::configManager.load(defConfig);
    core::configManager.enableAutoSave();
    core::configManager.acquire();

    // Fix missing elements in config
    for (auto const& item : defConfig.items()) {
        if (!core::configManager.conf.contains(item.key())) {
            flog::info("Missing key in config {0}, repairing", item.key());
            core::configManager.conf[item.key()] = defConfig[item.key()];
        }
    }

    // Remove unused elements
    auto items = core::configManager.conf.items();
    for (auto const& item : items) {
        if (!defConfig.contains(item.key())) {
            flog::info("Unused key in config {0}, repairing", item.key());
            core::configManager.conf.erase(item.key());
        }
    }

    core::configManager.release(true);

    if (serverMode) { return server::main(); }
    else {
        flog::error("This release is for server only, to get a GUI version, download SDR++");
        return 1;
    }
    flog::info("Exiting successfully");
    return 0;
}
