#include <arch/x86_64/cpu/cpu.h>
#include <sys/printk.h>
#include <mm/string.h>
#include <cpuid.h>
#include <stdint.h>
#include <cdefs.h>

#define PREFIX "[x86_64 CPU] "
#define CPUID_VENDOR_AMD           "AuthenticAMD"
#define CPUID_VENDOR_INTEL         "GenuineIntel"
#define CPUID_VENDOR_QEMU          "TCGTCGTCGTCG"
#define CPUID_VENDOR_KVM           " KVMKVMKVM  "
#define CPUID_VENDOR_VMWARE        "VMwareVMware"
#define CPUID_VENDOR_VIRTUALBOX    "VBoxVBoxVBox"
#define CPUID_VENDOR_HYPERV        "Microsoft Hv"

enum CPU_VENDOR {
        CPU_VENDOR_AMD,
        CPU_VENDOR_INTEL,
        CPU_VENDOR_QEMU,
        CPU_VENDOR_KVM,
        CPU_VENDOR_VMWARE,
        CPU_VENDOR_VIRTUALBOX,
        CPU_VENDOR_HYPERV,
};

static uint8_t cpuVendors = 7;
static const char *cpuVendorNames[] = {
        CPUID_VENDOR_AMD,
        CPUID_VENDOR_INTEL,
        CPUID_VENDOR_QEMU,
        CPUID_VENDOR_KVM,
        CPUID_VENDOR_VMWARE,
        CPUID_VENDOR_VIRTUALBOX,
        CPUID_VENDOR_HYPERV,
};

namespace x86_64 {
        CPU_VENDOR vendor;

        static int __checkSSE() {
                unsigned int eax, unused, edx;
                __cpuid(1, eax, unused, unused, edx);
                return edx | CPUID_FEAT_EDX_SSE;
        }

	static int __checkAPIC() {
		unsigned int eax, unused, edx;
                __cpuid(1, eax, unused, unused, edx);
		return edx | CPUID_FEAT_EDX_APIC;
	}

        static CPU_VENDOR __getVendor() {
                uint32_t ebx, edx, ecx, unused;
                __cpuid(0, unused, ebx, edx, ecx);
                char model[13];
                model[0] = (uint8_t)ebx;
                model[1] = (uint8_t)(ebx >> 8);
                model[2] = (uint8_t)(ebx >> 16);
                model[3] = (uint8_t)(ebx >> 24);
                model[4] = (uint8_t)ecx;
                model[5] = (uint8_t)(ecx >> 8);
                model[6] = (uint8_t)(ecx >> 16);
                model[7] = (uint8_t)(ecx >> 24);
                model[8] = (uint8_t)edx;
                model[9] = (uint8_t)(edx >> 8);
                model[10] = (uint8_t)(edx >> 16);
                model[11] = (uint8_t)(edx >> 24);
                model[12] = '\0';

                printk(PREFIX "CPU vendor is: %s\n", model);
                if(strcmp(model, cpuVendorNames[0]) == 0)
                        return CPU_VENDOR_AMD;
                if(strcmp(model, cpuVendorNames[1]) == 0)
                        return CPU_VENDOR_INTEL;

        }

        const char *GetVendor() {
                return cpuVendorNames[vendor];
        }

        void CPUInit() {
                vendor = __getVendor();

                if(__checkSSE()) {
#ifdef CONFIG_HW_SSE
                        printk(PREFIX "SSE status: Active\n");
#else
			printk(PREFIX "SSE status: Present but not active\n");
#endif
                }

		if (__checkAPIC()) {
                        printk(PREFIX "APIC status: Present\n");
		} else {
                        printk(PREFIX "APIC status: Not present\n");
		}

        }
}
