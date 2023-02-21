#pragma once

struct interrupt_frame;

__attribute__((interrupt)) void PageFault_handler(interrupt_frame *frame);
__attribute__((interrupt)) void DoubleFault_handler(interrupt_frame *frame);
__attribute__((interrupt)) void GPFault_handler(interrupt_frame *frame);

__attribute__((interrupt)) void KeyboardInt_handler(interrupt_frame *frame);
__attribute__((interrupt)) void MouseInt_handler(interrupt_frame *frame);
__attribute__((interrupt)) void PITInt_handler(interrupt_frame *frame);

