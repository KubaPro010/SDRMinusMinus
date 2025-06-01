#pragma once
#include <config.h>
#include <module.h>
#include <module_com.h>
#include "command_args.h"
#include "source.h"

namespace core {
    SDRMM_EXPORT ConfigManager configManager;
    SDRMM_EXPORT ModuleManager moduleManager;
    SDRMM_EXPORT ModuleComManager modComManager;
    SDRMM_EXPORT CommandArgsParser args;
    SDRMM_EXPORT SourceManager sourceManager;

    void setInputSampleRate(double samplerate);
};

int sdrmm_main(int argc, char* argv[]);