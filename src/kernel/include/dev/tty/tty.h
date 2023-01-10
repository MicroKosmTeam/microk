#pragma once
#include <mm/heap.h>
#include <stdint.h>

class TTY {
public:
        TTY();
        ~TTY();
        bool Activate();
        void Deactivate();
        void SendChar(char ch);
private:
        void PrintPrompt();
        void ElaborateCommand();
        char *user_mask;
        bool is_active;
};

extern TTY GlobalTTY;
