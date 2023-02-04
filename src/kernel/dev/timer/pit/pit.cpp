#include <dev/timer/pit/pit.h>
#include <io/io.h>

namespace PIT {
        double TimeSinceBoot = 0;
        uint16_t Divisor = 65535; // Max value

        void ResetTimer() {
                TimeSinceBoot = 0;
        }

        void Sleepd(double seconds) {
                double startTime = TimeSinceBoot;
                while (TimeSinceBoot < startTime + seconds) {
                        asm volatile("hlt");
                }
        }

        void Sleep(uint64_t milliseconds) {
                Sleepd((double)milliseconds / (double)1000);
        }

        void SetDivisor(uint16_t divisor) {
                if (divisor < 100) divisor = 100;
                Divisor = divisor;

                outb(0x40, (uint8_t)(divisor & 0x00ff));
                io_wait();
                outb(0x40, (uint8_t)(divisor & 0xff00) >> 8);
                io_wait();
        }

        uint64_t GetFrequency() {
                return BaseFrequency / Divisor;
        }

        void SetFrequency(uint64_t frequency) {
                SetDivisor(BaseFrequency / frequency);
        }

        void Tick() {
                TimeSinceBoot += (double)1 / (double)GetFrequency();
        }
}
