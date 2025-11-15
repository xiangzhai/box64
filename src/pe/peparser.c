#define PACKAGE         1
#define PACKAGE_VERSION 1
#include <bfd.h>
#include <stdio.h>

#include "debug.h"
#include "peloader_private.h"

static bfd* m_bfd = NULL;

static const char* get_section_type(asection* section)
{
    flagword flags = section->flags;

    if (flags & SEC_CODE) return "CODE";
    if (flags & SEC_DATA) {
        if (flags & SEC_READONLY) return "RODATA";
        return "DATA";
    }
    if (flags & SEC_ALLOC && !(flags & SEC_LOAD)) return "BSS";
    if (flags & SEC_HAS_CONTENTS) return "DATA";
    if (flags & SEC_READONLY) return "RODATA";
    return "UNKNOWN";
}

peheader_t* ParsePeHeader32(FILE* f, const char* name, int exec)
{
    bfd_init();
    m_bfd = bfd_openr(name, NULL);
    if (m_bfd == NULL) {
        printf_log(LOG_INFO, "Error: fail to bfd_openr for name=%s\n", name);
        return NULL;
    }

    if (!bfd_check_format(m_bfd, bfd_object)) {
        printf_log(LOG_DEBUG, "Error: Not a recognized object file\n");
        goto END;
        return NULL;
    }

    printf_log(LOG_DEBUG, "=== PE File Information ===\n");
    printf_log(LOG_DEBUG, "File name: %s\n", bfd_get_filename(m_bfd));
    printf_log(LOG_DEBUG, "Format: %s\n", bfd_get_target(m_bfd));
    printf_log(LOG_DEBUG, "Architecture: %s\n",
        bfd_printable_arch_mach(bfd_get_arch(m_bfd), bfd_get_mach(m_bfd)));

    bfd_size_type size = bfd_get_size(m_bfd);
    printf_log(LOG_DEBUG, "File size: %lu bytes\n", (unsigned long)size);

    bfd_vma entry = bfd_get_start_address(m_bfd);
    printf_log(LOG_DEBUG, "Entry point: 0x%08lx\n", (unsigned long)entry);

    printf_log(LOG_DEBUG, "=== PE Characteristics ===\n");

    flagword flags = bfd_get_file_flags(m_bfd);
    printf_log(LOG_DEBUG, "File Flags: 0x%08x\n", (unsigned int)flags);

    if (flags & EXEC_P) {
        printf_log(LOG_DEBUG, "  - Executable file\n");
    }
    if (flags & HAS_RELOC) {
        printf_log(LOG_DEBUG, "  - Has relocations\n");
    }
    if (flags & HAS_LINENO) {
        printf_log(LOG_DEBUG, "  - Has line number information\n");
    }
    if (flags & HAS_DEBUG) {
        printf_log(LOG_DEBUG, "  - Has debugging information\n");
    }
    if (flags & HAS_SYMS) {
        printf_log(LOG_DEBUG, "  - Has symbols\n");
    }
    if (flags & HAS_LOCALS) {
        printf_log(LOG_DEBUG, "  - Has local symbols\n");
    }
    if (flags & DYNAMIC) {
        printf_log(LOG_DEBUG, "  - Dynamic executable\n");
    }
    if (flags & WP_TEXT) {
        printf_log(LOG_DEBUG, "  - Text section is write-protected\n");
    }
    if (flags & D_PAGED) {
        printf_log(LOG_DEBUG, "  - Demand paged\n");
    }

    asection* section;

    printf_log(LOG_DEBUG, "=== PE Sections ===\n");
    printf_log(LOG_DEBUG, "%-12s %-10s %-10s %s\n",
        "Name", "VMA", "Size", "Type");
    printf_log(LOG_DEBUG, "----------------------------------------\n");

    for (section = m_bfd->sections; section != NULL; section = section->next) {
        const char* name = bfd_section_name(section);
        bfd_vma vma = bfd_section_vma(section);
        bfd_size_type size = bfd_section_size(section);

        printf_log(LOG_DEBUG, "%-12s 0x%08lx 0x%06lx %s\n",
            name,
            (unsigned long)vma,
            (unsigned long)size,
            get_section_type(section));
    }

END:
    if (m_bfd) {
        bfd_close(m_bfd);
        m_bfd = NULL;
    }

    return NULL;
}
