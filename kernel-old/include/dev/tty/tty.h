#pragma once
#include <mm/heap.h>
#include <stdint.h>

class TTY {
public:
        TTY();
        ~TTY();
        void Activate();
        void Deactivate();
	void GetChar();
private:
        void PrintPrompt();
        void ElaborateCommand();
        char *user_mask;
        bool is_active;
        bool exit;
};

extern TTY *GlobalTTY;

