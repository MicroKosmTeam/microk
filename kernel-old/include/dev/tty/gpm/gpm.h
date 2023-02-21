#pragma once
#include <stdint.h>
#include <sys/math.h>

#define PS2XSIGN 0b00010000
#define PS2YSIGN 0b00100000
#define PS2XOVER 0b01000000
#define PS2YOVER 0b10000000

#define PS2LFBUT 0b00000001
#define PS2RTBUT 0b00000010
#define PS2MDBUT 0b00000100

extern uint8_t mouse_pointer[];

void InitPS2Mouse();
void HandlePS2Mouse(uint8_t data);
void ProcessMousePacket();

extern Point mouse_position;
