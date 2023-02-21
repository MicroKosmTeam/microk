#pragma once
#include <stdint.h>

class ModuleManager {
public:
        ModuleManager();
        ~ModuleManager();
        bool FindModule(char *moduleName);
        bool LoadModule(char *moduleName);
        void LoadELF(uint8_t *data);
private:
};

extern ModuleManager *GlobalModuleManager;
