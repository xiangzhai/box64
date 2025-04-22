#include "core.h"

int main(int argc, const char **argv, char **env) {

    x64emu_t* emu = NULL;
    elfheader_t* elf_header = NULL;
    elfheader_t* interp_header = NULL;
    if (initialize(argc, argv, env, &emu, &elf_header, &interp_header, 1)) {
        return -1;
    }

    if (interp_header)
        emulate(emu, interp_header);
    return emulate(emu, elf_header);
}
