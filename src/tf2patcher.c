/*!
 * tf2patcher.c
 * TF2 decal tool patcher
 * (c) default-username 2020
 */

#include "base.h"
#include "common.h"
#include "memory.h"


// platform-dependent code
// TODO implement these for linux
#ifdef WINDOWS

    #include <tlhelp32.h>

    // find tf2 process id by executable name (tf_win.exe / tf_win64.exe)
    static DWORD find_tf2_process_id(void)
    {
        DWORD pid = 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) {
            return 0;
        }

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        if (Process32First(snap, &pe)) {
            do {
                if (!strcasecmp(pe.szExeFile, "tf_win64.exe") || !strcasecmp(pe.szExeFile, "tf_win.exe") ||
                        !strcasecmp(pe.szExeFile, "hl2.exe")) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32Next(snap, &pe));
        }

        CloseHandle(snap);
        return pid;
    }


    // wait until tf2 process shows up
    DWORD get_tf2_process_id(void)
    {
        DWORD pid = find_tf2_process_id();
        if (!pid) {
            printf("Waiting for TF2 to start...\n");

            do {
                Sleep(1000);
                pid = find_tf2_process_id();
            } while (!pid);
        }

        verbose_print("Found TF2 process with PID %u\n", (unsigned int)pid);
        return pid;
    }


    // find client.dll in tf2 module table
    // wait for max 15 seconds if it's not yet loaded
    HMODULE get_tf2_client_module(HANDLE process)
    {
        HMODULE modules[8192];
        DWORD num_modules;
        char module_name[PATH_MAX], tempbuf[PATH_MAX], error_buf[8192];
        size_t num_retry = 0;

        do {
            if (!EnumProcessModulesEx(process, modules, sizeof(modules), &num_modules, LIST_MODULES_ALL)) {
                fprintf(stderr, "Cannot enumerate TF2 modules: %s\n", describe_error(error_buf, sizeof(error_buf)));
                return 0;
            } else {
                assert(num_modules % sizeof(HMODULE) == 0);
                num_modules /= sizeof(HMODULE);

                verbose_print("Found %u modules in TF2 process\n", num_modules);

                for (size_t i = 0; i < num_modules; i++) {
                    if (GetModuleFileNameEx(process, modules[i], module_name, sizeof(module_name)) &&
                            !strcasecmp(extract_file_name(module_name, tempbuf, sizeof(tempbuf)), "client.dll")) {
                        verbose_print("Found TF2 client.dll (%zu/%u)\n", i + 1, num_modules);
                        return modules[i];
                    }
                }
            }

            Sleep(1000);
        } while (num_retry++ <= 30);

        fprintf(stderr, "Failed to find TF2 client.dll module!\n");
        return 0;
    }


    bool attach_to_tf2(void)
    {
        char error_buf[8192];

        DWORD process_id = get_tf2_process_id();
        HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id);

        if (!process) {
            fprintf(stderr, "Cannot attach to TF2 process: %s\n", describe_error(error_buf, sizeof(error_buf)));
        } else {
            verbose_print("Attached to TF2 process with PID %u\n", process_id);

            HMODULE module = get_tf2_client_module(process);
            if (module) {
                pinfo.process = process;
                pinfo.module = module;

                return true;
            }

            CloseHandle(process);
        }

        return false;
    }


    bool calc_client_module_bounds(void)
    {
        pinfo.cl_base = (unsigned char *)pinfo.module;

        IMAGE_DOS_HEADER dos_hdr;
        IMAGE_NT_HEADERS nt_hdr;

        if (read_mem(pinfo.cl_base, &dos_hdr, sizeof(dos_hdr)) &&
            read_mem(pinfo.cl_base + dos_hdr.e_lfanew, &nt_hdr, sizeof(nt_hdr))) {

            // PE32+ (0x20B) means the game is running as 64-bit
            pinfo.is64 = nt_hdr.OptionalHeader.Magic == 0x20B;
            pinfo.cl_size = nt_hdr.OptionalHeader.SizeOfImage;
            verbose_print("TF2 client.dll module: 0x%" PRIXPTR " with sz=%zu\n",
                    (uintptr_t)pinfo.cl_base, pinfo.cl_size);
            return true;
        }

        fprintf(stderr, "Failed to calculate TF2 client.dll module size!\n");
        return false;
    }


    void free_resources(void)
    {
        if (pinfo.process) {
            CloseHandle(pinfo.process);
            pinfo.process = 0;
        }
    }

#endif






// perform new patching method thanks to leaked tf2 source code
bool do_patch(void)
{
    printf("Patching...\n");

    // CConfirmCustomizeTextureDialog::PerformFilter
    unsigned char pattern32[] = {
        0x6A, 0x04,
        0x8B, 0x01,
        0xFF, 0x90, 0xFF, 0xFF, 0x00, 0x00, // mask first two bytes in opcode param
        0x8B, 0x8E, 0xFF, 0xFF, 0x00, 0x00, // same here
        0xE8, 0xFF, 0xFF, 0xFF, 0xFF,       // mask call parm
        0x83, 0xE8, 0x00
    };
    unsigned char pattern64[] = {
        0xBA, 0x04, 0x00, 0x00, 0x00,
        0x48, 0x8B, 0x01,
        0xFF, 0x90, 0xFF, 0xFF, 0x00, 0x00, // mask vtable offset
        0x48, 0x8B, 0x8B, 0xFF, 0xFF, 0x00, 0x00, // mask member offset
        0xE8, 0xFF, 0xFF, 0xFF, 0xFF,       // mask call parm
        0x85, 0xC0,
        0x74, 0xFF,
        0x83, 0xE8, 0x01
    };

    const unsigned char *pattern = pinfo.is64 ? pattern64 : pattern32;
    const size_t pattern_size = pinfo.is64 ? sizeof(pattern64) : sizeof(pattern32);
    const size_t call_offset = pinfo.is64 ? 21 : 16;
    printf("Target is %s TF2\n", pinfo.is64 ? "64-bit" : "32-bit");

    unsigned char *addr = find_mem_cl(pattern, pattern_size);
    if (!addr) {
        // maybe we already patched this session: the call is a mov eax, 1 now
        unsigned char patched[sizeof(pattern64)];
        memcpy(patched, pattern, pattern_size);
        memcpy(patched + call_offset, (unsigned char []){0xB8, 0x01, 0x00, 0x00, 0x00}, 5);

        if (find_mem_cl(patched, pattern_size)) {
            printf("TF2 is already patched. Restart TF2 if you want to undo it.\n");
            return true;
        }
    }
    if (!addr) {
        fprintf(stderr, "Failed to find CConfirmCustomizeTextureDialog::PerformFilter pattern in client library!\n");
    } else {
        verbose_print("CConfirmCustomizeTextureDialog::PerformFilter pattern addr: 0x%" PRIXPTR "\n", (uintptr_t)addr);

        // rewrite call to mov in order to force identity filter
        addr += call_offset;
        set_mem(addr, (unsigned char []){0xB8, 0x01, 0x00, 0x00, 0x00}, 5);
        verbose_print("Rewrote CALL to MOV\n");

        // disable blending
        // check memory first
        unsigned char pattern2[] = {
            0x80, 0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
            0x74
        };

        addr = find_mem(pattern2, sizeof(pattern2), addr, 200);
        if (!addr) {
            fprintf(stderr, "Failed to find CConfirmCustomizeTextureDialog::PerformFilter pattern #2 in client library!\n");
        } else {
            // rewrite jz to jmp
            addr += 7;
            set_mem(addr, (unsigned char []){0xEB}, 1);
            verbose_print("Rewrote JZ to JMP\n");

            // and thats pretty much it
            printf("Done!\n");
            return true;
        }
    }

    return false;
}




int main(int argc, char *argv[])
{
    printf(
        "  --------------------------------------------\n"
        "  |       TF2 decal tool patcher 2.0.2       |\n"
        "  | (c) default-username, Apr 2020, Mar 2016 |\n"
        "  --------------------------------------------\n"
        "\n");

    if (argv) {
        for (int i = 0; i < argc; i++) {
            if (argv[i]) {
                if (!strcasecmp(argv[i], "-h") || !strcasecmp(argv[i], "--help")) {
                    printf(
                        "Usage: tf2patcher -[hv]\n"
                        "       -h | --help    : show this help\n"
                        "       -v | --verbose : be more verbose\n");
                    return EXIT_SUCCESS;
                } else if (!strcasecmp(argv[i], "-v") || !strcasecmp(argv[i], "--verbose")) {
                    pinfo.verbose_mode = true;
                }
            }
        }
    }

    if (pinfo.verbose_mode) {
        printf("Running in verbose mode\n");
    }

    bool res = attach_to_tf2() && calc_client_module_bounds() && do_patch();

    free_resources();

    printf("Press ENTER to exit...\n");
    getchar();

    return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
