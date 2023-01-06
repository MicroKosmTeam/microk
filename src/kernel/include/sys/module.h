#pragma once
#include <stdint.h>

class ModuleManager {
public:
        ModuleManager();
        ~ModuleManager();
        bool FindModule(char *moduleName);
        bool LoadModule(char *moduleName);
private:
};

extern ModuleManager GlobalModuleManager;
