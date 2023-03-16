#pragma once

extern "C" void __stack_chk_fail(void);
void UnwindStack(int MaxFrame);
