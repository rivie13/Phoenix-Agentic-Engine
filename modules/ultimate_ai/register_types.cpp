#include "register_types.h"

void initialize_ultimate_ai_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_EDITOR) {
        return;
    }

    // TODO: Register classes for the Ultimate AI module.
}

void uninitialize_ultimate_ai_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_EDITOR) {
        return;
    }

    // TODO: Unregister classes for the Ultimate AI module.
}
