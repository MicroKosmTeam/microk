#include <sys/symbols.h>
const uint64_t symbolCount = 196;
const symbol_t symbols[] = {
   {{0xffffffff80000000}, {"_ZN6x86_649GetVendorEv"}}, 
   {{0xffffffff80000000}, {"kernel_start"}}, 
   {{0xffffffff800000e0}, {"_ZN6x86_647CPUInitEv"}}, 
   {{0xffffffff8000021c}, {"_ZN6x86_649GetVendorEv.cold"}}, 
   {{0xffffffff80000230}, {"_Z8gdt_initv"}}, 
   {{0xffffffff80000310}, {"_Z8tss_initv"}}, 
   {{0xffffffff800003b0}, {"_Z13tss_set_stackm"}}, 
   {{0xffffffff800003e0}, {"_ZN6x86_6410SetIDTGateEPvhhh"}}, 
   {{0xffffffff800004c0}, {"_ZN6x86_647LoadGDTEv"}}, 
   {{0xffffffff800004f0}, {"_ZN6x86_6417PrepareInterruptsEP8BootInfo"}}, 
   {{0xffffffff80000760}, {"_ZN6x86_6413PreparePagingEPP19limine_memmap_entrymm"}}, 
   {{0xffffffff80000790}, {"_ZN12IDTDescEntry9SetOffsetEm"}}, 
   {{0xffffffff80000880}, {"_ZN12IDTDescEntry9GetOffsetEv"}}, 
   {{0xffffffff80000970}, {"_Z17PageFault_handlerP15interrupt_frame"}}, 
   {{0xffffffff800009b0}, {"_Z19DoubleFault_handlerP15interrupt_frame"}}, 
   {{0xffffffff800009f0}, {"_Z15GPFault_handlerP15interrupt_frame"}}, 
   {{0xffffffff80000a30}, {"_Z4outbth"}}, 
   {{0xffffffff80000a60}, {"_Z4outwtt"}}, 
   {{0xffffffff80000a90}, {"_Z3inbt"}}, 
   {{0xffffffff80000ac0}, {"_Z3inwt"}}, 
   {{0xffffffff80000af0}, {"_Z7io_waitv"}}, 
   {{0xffffffff80000b20}, {"_Z9flush_tlbm"}}, 
   {{0xffffffff80000b50}, {"_ZN3VMM24InitVirtualMemoryManagerEPP19limine_memmap_entrymm"}}, 
   {{0xffffffff80000c80}, {"_ZN3VMM9MapMemoryEPvS0_"}}, 
   {{0xffffffff80001320}, {"_ZN4ACPI9FindTableEPNS_9SDTHeaderEPc"}}, 
   {{0xffffffff800016b0}, {"_ZN3PCI17EnumerateFunctionEmm"}}, 
   {{0xffffffff800018d0}, {"_ZN3PCI15EnumerateDeviceEmm"}}, 
   {{0xffffffff80001b60}, {"_ZN3PCI12EnumerateBusEmm"}}, 
   {{0xffffffff80001e70}, {"_ZN3PCI12EnumeratePCIEPN4ACPI10MCFGHeaderE"}}, 
   {{0xffffffff800022a0}, {"_ZN3PCI13GetVendorNameEt"}}, 
   {{0xffffffff80002330}, {"_ZN3PCI13GetDeviceNameEtt"}}, 
   {{0xffffffff800023e0}, {"_ZN3PCI33MassStorageControllerSubclassNameEh"}}, 
   {{0xffffffff800024f0}, {"_ZN3PCI31SerialBusControllerSubclassNameEh"}}, 
   {{0xffffffff80002610}, {"_ZN3PCI24BridgeDeviceSubclassNameEh"}}, 
   {{0xffffffff80002730}, {"_ZN3PCI15GetSubclassNameEhh"}}, 
   {{0xffffffff80002a00}, {"_ZN3PCI13GetProgIFNameEhhh"}}, 
   {{0xffffffff80002b50}, {"_ZN10UARTDevice4InitE11SerialPorts"}}, 
   {{0xffffffff80002ef0}, {"_ZN10UARTDevice6PutStrEPKc"}}, 
   {{0xffffffff800033c0}, {"_ZN10UARTDevice5ioctlEmz"}}, 
   {{0xffffffff80003780}, {"_ZN10UARTDevice7PutCharEc"}}, 
   {{0xffffffff800038f0}, {"_ZN10UARTDevice7GetCharEv"}}, 
   {{0xffffffff80003a10}, {"_ZN10UARTDevice15isTransmitEmptyEv"}}, 
   {{0xffffffff80003a90}, {"_ZN10UARTDevice14serialReceivedEv"}}, 
   {{0xffffffff80003b0a}, {"_ZN10UARTDevice6PutStrEPKc.cold"}}, 
   {{0xffffffff80003b62}, {"_ZN10UARTDevice5ioctlEmz.cold"}}, 
   {{0xffffffff80003b92}, {"_ZN10UARTDevice7PutCharEc.cold"}}, 
   {{0xffffffff80003ba8}, {"_ZN10UARTDevice7GetCharEv.cold"}}, 
   {{0xffffffff80003bc0}, {"_Z7smpInitP15limine_smp_info"}}, 
   {{0xffffffff80003bf0}, {"_Z5kinitP8BootInfo"}}, 
   {{0xffffffff80004ac0}, {"_Z8restInitv"}}, 
   {{0xffffffff80004ae0}, {"_ZN6Device5ioctlEmz"}}, 
   {{0xffffffff80004b0c}, {"_Z5kinitP8BootInfo.cold"}}, 
   {{0xffffffff80004b20}, {"_start"}}, 
   {{0xffffffff800050a0}, {"_ZN6BitmapixEm"}}, 
   {{0xffffffff800051f0}, {"_ZN6Bitmap3getEm"}}, 
   {{0xffffffff800052f0}, {"_ZN6Bitmap3setEmb"}}, 
   {{0xffffffff80005530}, {"_ZN13HeapSegHeader14CombineForwardEv"}}, 
   {{0xffffffff800058b0}, {"_ZN13HeapSegHeader15CombineBackwardEv"}}, 
   {{0xffffffff80005d40}, {"_ZN13HeapSegHeader5SplitEm"}}, 
   {{0xffffffff80006130}, {"_Z14InitializeHeapPvm"}}, 
   {{0xffffffff800062f0}, {"free"}}, 
   {{0xffffffff80006b30}, {"_Z10ExpandHeapm"}}, 
   {{0xffffffff80007150}, {"malloc"}}, 
   {{0xffffffff8000781c}, {"_ZN13HeapSegHeader14CombineForwardEv.cold"}}, 
   {{0xffffffff80007832}, {"_ZN13HeapSegHeader15CombineBackwardEv.cold"}}, 
   {{0xffffffff8000785c}, {"_ZN13HeapSegHeader5SplitEm.cold"}}, 
   {{0xffffffff80007879}, {"free.cold"}}, 
   {{0xffffffff800078b8}, {"_Z10ExpandHeapm.cold"}}, 
   {{0xffffffff800078e1}, {"malloc.cold"}}, 
   {{0xffffffff80007920}, {"_Z15get_memory_sizePP19limine_memmap_entrym"}}, 
   {{0xffffffff80007a70}, {"memcmp"}}, 
   {{0xffffffff80007ba0}, {"memset"}}, 
   {{0xffffffff80007bd0}, {"memcpy"}}, 
   {{0xffffffff80007cf0}, {"_ZN3PMM22InitPageFrameAllocatorEPP19limine_memmap_entrym"}}, 
   {{0xffffffff800086f0}, {"_ZN3PMM11RequestPageEv"}}, 
   {{0xffffffff80008820}, {"_ZN3PMM12RequestPagesEm"}}, 
   {{0xffffffff800089f0}, {"_ZN3PMM8FreePageEPv"}}, 
   {{0xffffffff80008a80}, {"_ZN3PMM8LockPageEPv"}}, 
   {{0xffffffff80008b00}, {"_ZN3PMM9FreePagesEPvm"}}, 
   {{0xffffffff80008c40}, {"_ZN3PMM9LockPagesEPvm"}}, 
   {{0xffffffff80008d60}, {"_ZN3PMM10GetFreeMemEv"}}, 
   {{0xffffffff80008d90}, {"_ZN3PMM10GetUsedMemEv"}}, 
   {{0xffffffff80008dc0}, {"_ZN3PMM14GetReservedMemEv"}}, 
   {{0xffffffff80008dee}, {"_ZN3PMM22InitPageFrameAllocatorEPP19limine_memmap_entrym.cold"}}, 
   {{0xffffffff80008e10}, {"_Z6strlenPKc"}}, 
   {{0xffffffff80008ec0}, {"_Z6strcpyPcPKc"}}, 
   {{0xffffffff80008fd0}, {"_Z6strcmpPKcS0_"}}, 
   {{0xffffffff80009140}, {"_Z8is_delimcPc"}}, 
   {{0xffffffff80009210}, {"_Z6strtokPcS_"}}, 
   {{0xffffffff800095d0}, {"_Z4itoaPcix"}}, 
   {{0xffffffff80009880}, {"_Z4atoiPc"}}, 
   {{0xffffffff80009aa0}, {"_Z9to_stringm"}}, 
   {{0xffffffff80009dc0}, {"_Z10to_hstringm"}}, 
   {{0xffffffff8000aca0}, {"_Z10to_hstringj"}}, 
   {{0xffffffff8000b360}, {"_Z10to_hstringt"}}, 
   {{0xffffffff8000b610}, {"_Z10to_hstringh"}}, 
   {{0xffffffff8000b6c0}, {"_Z9to_stringl"}}, 
   {{0xffffffff8000ba80}, {"_Z9to_stringdh"}}, 
   {{0xffffffff8000c1f0}, {"_Z9to_stringd"}}, 
   {{0xffffffff8000c8f0}, {"iscntrl"}}, 
   {{0xffffffff8000c930}, {"isdigit"}}, 
   {{0xffffffff8000c970}, {"isgraph"}}, 
   {{0xffffffff8000c9b0}, {"isprint"}}, 
   {{0xffffffff8000c9f0}, {"isspace"}}, 
   {{0xffffffff8000ca40}, {"isalpha"}}, 
   {{0xffffffff8000caa0}, {"islower"}}, 
   {{0xffffffff8000cb00}, {"isalnum"}}, 
   {{0xffffffff8000cb90}, {"isblank"}}, 
   {{0xffffffff8000cbd0}, {"ispunct"}}, 
   {{0xffffffff8000cc50}, {"isupper"}}, 
   {{0xffffffff8000ccb0}, {"isxdigit"}}, 
   {{0xffffffff8000ccf0}, {"tolower"}}, 
   {{0xffffffff8000cd70}, {"toupper"}}, 
   {{0xffffffff8000ce10}, {"__cxa_guard_acquire"}}, 
   {{0xffffffff8000ce60}, {"__cxa_guard_release"}}, 
   {{0xffffffff8000ceb0}, {"__cxa_guard_abort"}}, 
   {{0xffffffff8000cee0}, {"__cxa_atexit"}}, 
   {{0xffffffff8000d150}, {"__cxa_finalize"}}, 
   {{0xffffffff8000d690}, {"_Z5panikPKcS0_S0_j"}}, 
   {{0xffffffff8000d750}, {"_Z6printkPcz"}}, 
   {{0xffffffff8000ea60}, {"_Z18printk_init_serialP10UARTDevice"}}, 
   {{0xffffffff8000ed61}, {"_Z6printkPcz.cold"}}, 
   {{0xffffffff8000ee4f}, {"_Z18printk_init_serialP10UARTDevice.cold"}}, 
   {{0xffffffff8000ee70}, {"__stack_chk_fail"}}, 
   {{0xffffffff8000eeb0}, {"UnwindStack"}}, 
   {{0xffffffff8000f0b0}, {"_Z13lookup_symbolm"}}, 
   {{0xffffffff8000f1c0}, {"__ubsan_handle_missing_return"}}, 
   {{0xffffffff8000f1f0}, {"__ubsan_handle_divrem_overflow"}}, 
   {{0xffffffff8000f220}, {"__ubsan_handle_load_invalid_value"}}, 
   {{0xffffffff8000f250}, {"__ubsan_handle_shift_out_of_bounds"}}, 
   {{0xffffffff8000f280}, {"__ubsan_handle_builtin_unreachable"}}, 
   {{0xffffffff8000f2b0}, {"__ubsan_handle_mul_overflow"}}, 
   {{0xffffffff8000f2e0}, {"__ubsan_handle_negate_overflow"}}, 
   {{0xffffffff8000f310}, {"__ubsan_handle_out_of_bounds"}}, 
   {{0xffffffff8000f3c0}, {"__ubsan_handle_pointer_overflow"}}, 
   {{0xffffffff8000f3f0}, {"__ubsan_handle_add_overflow"}}, 
   {{0xffffffff8000f420}, {"__ubsan_handle_sub_overflow"}}, 
   {{0xffffffff8000f450}, {"__ubsan_handle_type_mismatch_v1"}}, 
   {{0xffffffff8000f820}, {"gdt_flush"}}, 
   {{0xffffffff8000f83b}, {"tss_flush"}}, 
   {{0xffffffff8000f850}, {"PushPageDir"}}, 
   {{0xffffffff8000f854}, {"invalidate_tlb"}}, 
   {{0xffffffff80011400}, {"CSWTCH.7"}}, 
   {{0xffffffff80011440}, {"_ZTV10UARTDevice"}}, 
   {{0xffffffff80011760}, {"_ZTV6Device"}}, 
   {{0xffffffff80011920}, {"CSWTCH.44"}}, 
   {{0xffffffff80012cc0}, {"symbols"}}, 
   {{0xffffffff80013900}, {"symbolCount"}}, 
   {{0xffffffff80016380}, {"_ZL14cpuVendorNames"}}, 
   {{0xffffffff800163c0}, {"gdt"}}, 
   {{0xffffffff80017200}, {"_ZN3PCI13DeviceClassesE"}}, 
   {{0xffffffff80018900}, {"_ZL19kernel_file_request"}}, 
   {{0xffffffff80018940}, {"_ZL11smp_request"}}, 
   {{0xffffffff80018980}, {"_ZL14module_request"}}, 
   {{0xffffffff800189c0}, {"_ZL12hhdm_request"}}, 
   {{0xffffffff80018a00}, {"_ZL12rsdp_request"}}, 
   {{0xffffffff80018a40}, {"_ZL19framebuffer_request"}}, 
   {{0xffffffff80018a80}, {"_ZL14memmap_request"}}, 
   {{0xffffffff80018ac0}, {"_ZL18stack_size_request"}}, 
   {{0xffffffff80018b00}, {"_ZL23bootloader_info_request"}}, 
   {{0xffffffff8001d9f8}, {"__stack_chk_guard"}}, 
   {{0xffffffff8001dd80}, {"Type_Check_Kinds"}}, 
   {{0xffffffff8001dde0}, {"_ZN6x86_646vendorE"}}, 
   {{0xffffffff8001de00}, {"tss"}}, 
   {{0xffffffff8001de68}, {"gdt_pointer"}}, 
   {{0xffffffff8001de78}, {"_ZN6x86_644idtrE"}}, 
   {{0xffffffff8001de88}, {"pageDir"}}, 
   {{0xffffffff8001de90}, {"rootNode"}}, 
   {{0xffffffff8001de98}, {"kernelStack"}}, 
   {{0xffffffff8001dea0}, {"bootInfo"}}, 
   {{0xffffffff8001df08}, {"lastHeader"}}, 
   {{0xffffffff8001df10}, {"heapEnd"}}, 
   {{0xffffffff8001df18}, {"heapStart"}}, 
   {{0xffffffff8001df20}, {"_ZZ15get_memory_sizePP19limine_memmap_entrymE17memory_size_bytes"}}, 
   {{0xffffffff8001df30}, {"_ZL17page_bitmap_index"}}, 
   {{0xffffffff8001df40}, {"_ZL11page_bitmap"}}, 
   {{0xffffffff8001df50}, {"_ZL11initialized"}}, 
   {{0xffffffff8001df58}, {"_ZL11used_memory"}}, 
   {{0xffffffff8001df60}, {"_ZL15reserved_memory"}}, 
   {{0xffffffff8001df68}, {"_ZL11free_memory"}}, 
   {{0xffffffff8001df70}, {"_ZZ6strtokPcS_E1p"}}, 
   {{0xffffffff8001df80}, {"doubleTo_StringOutput"}}, 
   {{0xffffffff8001e000}, {"intTo_StringOutput"}}, 
   {{0xffffffff8001e080}, {"hexTo_StringOutput8"}}, 
   {{0xffffffff8001e100}, {"hexTo_StringOutput16"}}, 
   {{0xffffffff8001e180}, {"hexTo_StringOutput32"}}, 
   {{0xffffffff8001e200}, {"hexTo_StringOutput"}}, 
   {{0xffffffff8001e280}, {"uintTo_StringOutput"}}, 
   {{0xffffffff8001e300}, {"__dso_handle"}}, 
   {{0xffffffff8001e308}, {"__atexit_func_count"}}, 
   {{0xffffffff8001e320}, {"__atexit_funcs"}}, 
   {{0xffffffff8001ef20}, {"serial"}}, 
   {{0xffffffff8001ef40}, {"earlyPrintkBuffer"}}, 
   {{0xffffffff80022f40}, {"printkSerial"}}, 
   {{0xffffffff80022f48}, {"_ZL14earlyPrintkPos"}}, 
   {{0xffffffff80022f4c}, {"kernel_end"}}, 
};
