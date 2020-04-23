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

	// acquire hwnd to tf2 window
	// wait if the window is not there yet
	HWND get_tf2_window(void)
	{
		const char *window_name = "Team Fortress 2";

		HWND window = FindWindow(NULL, window_name);
		if (!window) {
			printf("Waiting for TF2 to start...\n");

			do {
				Sleep(1000);
				window = FindWindow(NULL, window_name);
			} while (!window);
		}

		verbose_print("Found TF2 window with HWND %u\n", window);
		return window;
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

				for (unsigned int i = 0; i < num_modules; i++) {
					if (GetModuleFileNameEx(process, modules[i], module_name, sizeof(module_name)) &&
							!strcasecmp(extract_file_name(module_name, tempbuf, sizeof(tempbuf)), "client.dll")) {
						verbose_print("Found TF2 client.dll (%u/%u)\n", i + 1, num_modules);
						return modules[i];
					}
				}
			}

			Sleep(1000);
		} while (num_retry++ <= 15);

		fprintf(stderr, "Failed to find TF2 client.dll module!\n");
		return 0;
	}


	bool attach_to_tf2(void)
	{
		char error_buf[8192];

		HWND window = get_tf2_window();

		DWORD process_id;
		GetWindowThreadProcessId(window, &process_id);
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
			pinfo.cl_size = nt_hdr.OptionalHeader.SizeOfImage;
			verbose_print("TF2 client.dll module: %#x with sz=%#x\n", pinfo.cl_base, pinfo.cl_size);
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
	unsigned char pattern[] = {0x6A, 0x04,
														 0x8B, 0x01,
														 0xFF, 0x90, 0xFF, 0xFF, 0x00, 0x00, // mask first two bytes in opcode param
														 0x8B, 0x8E, 0xFF, 0xFF, 0x00, 0x00, // same here
														 0xE8, 0xFF, 0xFF, 0xFF, 0xFF,       // mask call parm
														 0x83, 0xE8, 0x00};


	unsigned char *addr = find_mem_cl(pattern, sizeof(pattern));
	if (!addr) {
		fprintf(stderr, "Failed to find CConfirmCustomizeTextureDialog::PerformFilter pattern in client library!\n");
	} else {
		verbose_print("CConfirmCustomizeTextureDialog::PerformFilter pattern addr: %p\n", addr);

		// rewrite call to mov in order to force identity filter
		addr += 16;
		set_mem(addr, (unsigned char[]){0xB8, 0x01, 0x00, 0x00, 0x00}, 5);
		verbose_print("Rewrote CALL to MOV\n");


		// disable blending
		// check memory first
		unsigned char pattern2[] = {0x80, 0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
																0x74};

		addr = find_mem(pattern2, sizeof(pattern2), addr, 200);
		if (!addr) {
			fprintf(stderr, "Failed to find CConfirmCustomizeTextureDialog::PerformFilter pattern #2 in client library!\n");
		} else {
			// rewrite jz to jmp
			addr += 7;
			set_mem(addr, (unsigned char[]){0xEB}, 1);
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
	printf("  --------------------------------------------\n"
				 "  |       TF2 decal tool patcher 2.0.0       |\n"
				 "  | (c) default-username, Apr 2020, Mar 2016 |\n"
				 "  --------------------------------------------\n"
				 "\n");

	if (argv) {
		for (int i = 0; i < argc; i++) {
			if (argv[i]) {
				if (!strcasecmp(argv[i], "-h") || !strcasecmp(argv[i], "--help")) {
					printf("Usage: tf2patcher -[hv]\n"
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

	return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
