#include <sys/module.h>
#include <mm/heap.h>

ModuleManager GlobalModuleManager;

ModuleManager::ModuleManager() {

}

ModuleManager::~ModuleManager() {

}

bool ModuleManager::FindModule(char *moduleName) {
        //Unimplemented
        return false;
}

bool ModuleManager::LoadModule(char *moduleName) {
        //Unimplemented
        return false;
}
