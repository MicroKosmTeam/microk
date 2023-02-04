#include <dev/tty/tty.h>
#include <dev/fb/gop.h>
#include <sys/panik.h>
#include <sys/printk.h>
#include <mm/memory.h>
#include <module/module.h>
#include <mm/string.h>
#include <mm/pageframe.h>
#include <sys/cstr.h>
#include <dev/timer/pit/pit.h>
#include <fs/fs.h>
#include <kutil.h>
#include <fs/vfs.h>
#include <demo.h>
#include <cpu/cpu.h>
#include <proc/scheduler.h>

#define PREFIX "[TTY] "

TTY *GlobalTTY;

TTY::TTY() {
	dprintf(PREFIX "Initializing TTY...\n");
	is_active = false;
	exit = false;
}

TTY::~TTY() {
	dprintf(PREFIX "Destroying TTY...\n");
}

void TTY::Activate() {
	dprintf(PREFIX "Activating TTY...\n\n");
	user_mask = (char*)malloc(512);
	memset(user_mask, ' ', 512);

	PrintPrompt();

	// We activate it at the end
	is_active = true;

	/*while(is_active) {
		GetChar();
	}*/

	return;
}

void TTY::Deactivate() {
	dprintf(PREFIX "Dectivating TTY...\n");
	// We deactivate it immediatly
	is_active = false;
	exit = true;

	free(user_mask);
	return;
}

void TTY::PrintPrompt() {
	printf("(kernel mode) $ ");
	fflush(stdout);
}

void TTY::ElaborateCommand() {
	printf("\n");

	char *ptr = strtok(user_mask, " ");

	if (strcmp(ptr, "help") == 0) {
		printf("Available commands\n"
		       "cls\n"
		       "uname\n"
		       "module\n"
		       "panik\n"
		       "mem\n"
		       "heap\n"
		       "stack\n"
		       "time\n"
		       "demo\n"
		       "fetch\n"
		       "ls\n"
		       "top\n"
		       );
	} else if (strcmp(ptr, "cls") == 0 || strcmp(ptr, "clear") == 0 || strcmp(ptr, "clean") == 0) {
		GlobalRenderer.print_clear();
	} else if (strcmp(ptr, "uname") == 0) {
		printf("%s %s %s\n", KNAME, KVER, CPU::GetVendor());
	} else if (strcmp(ptr, "module") == 0) {
		ptr = strtok(NULL, " ");
		if (ptr != NULL) {
			if(GlobalModuleManager->FindModule(ptr)) {
				printf("Module found! Loading...\n");
				if(GlobalModuleManager->LoadModule(ptr)) {
					printf("Module loaded successfully.\n");
				} else {
					printf("Loading module failed!\n");
				}
			} else {
				printf("No module found: %s\n", ptr);
			}
		} else {
			printf("Insert a module name.\n");
		}
	} else if (strcmp(ptr, "panik") == 0) {
		PANIK("User induced panic.");
	} else if (strcmp(ptr, "mem") == 0) {
		printf("Memory Status:\n"
		       " -> Kernel:      %dkb.\n"
		       " -> Free:        %dkb.\n"
		       " -> Used:        %dkb.\n"
		       " -> Reserved:    %dkb.\n"
		       " -> Total:       %dkb.\n",
			kInfo.kernel_size / 1024,
			GlobalAllocator.GetFreeMem() / 1024,
			GlobalAllocator.GetUsedMem() / 1024,
			GlobalAllocator.GetReservedMem() / 1024,
			(GlobalAllocator.GetFreeMem() + GlobalAllocator.GetUsedMem()) / 1024);
	} else if (strcmp(ptr, "heap") == 0) {
		VisualizeHeap();
	} else if (strcmp(ptr, "stack") == 0) {
		uint16_t stackFrames;
		ptr = strtok(NULL, " ");
		if (ptr != NULL) {
			stackFrames = atoi(ptr);
		} else {
			stackFrames = 2;
		}
		UnwindStack(stackFrames);
	} else if (strcmp(ptr, "time") == 0) {
		printf("Ticks since initialization: %d.\n", (uint64_t)PIT::TimeSinceBoot);
	} else if (strcmp(ptr, "lsblk") == 0) {
		GlobalFSManager->ListDrives();
	} else if (strcmp(ptr, "exit") == 0) {
		scheduler_stop();
		GlobalTTY->Deactivate();
		return;
	} else if (strcmp(ptr, "demo") == 0) {
		ptr = strtok(NULL, " ");
		if (ptr != NULL) {
			if(strcmp(ptr, "badapple") == 0) {
				DEMO::BadApple();
			} else if(strcmp(ptr, "stress") == 0) {
				DEMO::Stress();
			} else if(strcmp(ptr, "raytrace") == 0) {
				DEMO::Raytrace();
			} else {
				printf("No demo found: %s\n", ptr);
			}
		} else {
			printf("Insert a demo name.\n");
		}
	} else if(strcmp(ptr, "fetch") == 0) {
		printf(" __  __  _                _  __    ___   ___\n"
		       "|  \\/  |(_) __  _ _  ___ | |/ /   / _ \\ / __|\n"
		       "| |\\/| || |/ _|| '_|/ _ \\|   <   | (_) |\\__ \\\n"
		       "|_|  |_||_|\\__||_|  \\___/|_|\\_\\   \\___/ |___/\n"
		       "The operating system from the future...at your fingertips.\n"
		       "\n"
		       " Memory Status:\n"
		       " -> Kernel:      %dkb.\n"
		       " -> Free:        %dkb.\n"
		       " -> Used:        %dkb.\n"
		       " -> Reserved:    %dkb.\n"
		       " -> Total:       %dkb.\n",
			kInfo.kernel_size / 1024,
			GlobalAllocator.GetFreeMem() / 1024,
			GlobalAllocator.GetUsedMem() / 1024,
			GlobalAllocator.GetReservedMem() / 1024,
			(GlobalAllocator.GetFreeMem() + GlobalAllocator.GetUsedMem()) / 1024);
	} else if(strcmp(ptr, "ls") == 0) {
		VFS_Tree();
	} else if(strcmp(ptr, "top") == 0) {
		scheduler_list();
	} else {
		printf("Unknown command: %s\n", ptr);
	}

	printf("\n");

}

void TTY::GetChar() {
	if (!is_active) return;

	char ch = getch();
	if (ch == '\0') return;

	static uint8_t curr_char;

	switch (ch) {
		case '\b':
			if (curr_char > 0) {
				user_mask[--curr_char] = NULL;
				GlobalRenderer.clear_char();
			}
			break;
		case '\r':
			break;
		case '\n':
			is_active = false;
			ElaborateCommand();

			if (exit) {
				return;
			} else {
				PrintPrompt();
				memset(user_mask, ' ', 512);
				curr_char = 0;
				is_active = true;
			}
			break;
		default:
			if (curr_char < 512) {
				user_mask[curr_char++] = ch;
				GlobalRenderer.print_char(ch);
			}
			break;
	}
}
