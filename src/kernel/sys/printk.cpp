#include <sys/printk.h>
#include <dev/serial/serial.h>

#define PREFIX "[PRINTK] "

static int earlyPrintkPos = 0;
static const int earlyPrintkSize = 16 * 1024;
char earlyPrintkBuffer[earlyPrintkSize] = { 0 }; // 16kb early

SerialPort printkSerial;
bool serial = false;

static void print_all(const char *string) {
	if (serial)
		printkSerial.PrintStr(string);
	else {
		char ch;
		while(ch = *string++) {
			if (earlyPrintkPos++ > earlyPrintkSize) return;
			earlyPrintkBuffer[earlyPrintkPos] = ch;
		}
	}
}

static void print_all_char(const char ch) {
	if (serial)
		printkSerial.PrintChar(ch);
	else
		if (earlyPrintkPos++ > earlyPrintkSize) return;
		earlyPrintkBuffer[earlyPrintkPos] = ch;
}

static void dump_early() {
	for (int i = 0; i < earlyPrintkPos; i++) {
		print_all_char(earlyPrintkBuffer[i]);
	}
	printk("\n");

	printk(PREFIX "Remaining available early printk memory: %d\n", earlyPrintkSize - earlyPrintkPos);
}

void printk_init_serial() {
	printkSerial = SerialPort(SerialPorts::COM1);
	serial = true;
	dump_early();
}

void printk(char *format, ...) {
        va_list ap;
        va_start(ap, format);

        char *ptr = format;
        char buf[128];

        while(*ptr) {
                if (*ptr == '%') {
                        ptr++;
                        switch (*ptr++) {
                                case 's':
                                        print_all(va_arg(ap, char *));
                                        break;
                                case 'd':
                                case 'u':
                                        itoa(buf, 'd', va_arg(ap, int64_t));
                                        print_all(buf);
                                        break;
                                case 'x':
                                        itoa(buf, 'x', va_arg(ap, int64_t));
					for (int i = 0; i < 18 - strlen(buf); i++) print_all_char('0');
                                        print_all(buf);
                                        break;
                                case '%':
                                        print_all_char('%');
                                        break;
                                case 'c':
                                        print_all_char((va_arg(ap, int32_t)));
                                        break;

                        }
                } else {
                        print_all_char(*ptr++);
                }
        }

        va_end(ap);
}
