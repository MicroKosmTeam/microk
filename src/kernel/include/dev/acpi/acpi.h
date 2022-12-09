#pragma once
#include <stdint.h>
#include <cdefs.h>

struct acpi_info {
        int num_cpus;
        
        struct {
                uint8_t id;
                uint8_t apic;
        } cpu[KMAX_CPUS];

        int num_ioapic;
        
        struct {
                uint8_t id;
                uint32_t addr;
                uint32_t base;
        } ioapic[KMAX_IOAPIC];

        uint32_t int_map[256];
};

extern struct acpi_info acpi_info;

void acpi_init();
int get_cpu();
