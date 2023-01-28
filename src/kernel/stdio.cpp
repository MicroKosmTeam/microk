#include <stdio.h>
#include <mm/string.h>

void fputc(char c, FILE *file) {
        VFS_Write(file, (uint8_t*)&c, sizeof(c));
}

void fputs(const char *str, FILE *file) {
        while(*str) {
                fputc(*str, file);
                str++;
        }
}

void fprintf(fd_t file, const char *format, ...) {
        va_list ap;
        va_start(ap, format);

	if(file == VFS_FILE_STDOUT) vfprintf(stdout, format, ap);
	else vfprintf(stdlog, format, ap);
        va_end(ap);
}

void vfprintf(FILE *file, const char *format, va_list args) {
        char *ptr = (char*)format;
        char buf[128];

        while(*ptr) {
                if (*ptr == '%') {
                        ptr++;
                        switch (*ptr++) {
                                case 's':
                                        fputs(va_arg(args, char *), file);
                                        break;
                                case 'd':
                                case 'u':
                                        itoa(buf, 'd', va_arg(args, long long int));
                                        fputs(buf, file);
                                        break;
                                case 'x':
                                        itoa(buf, 'x', va_arg(args, long long int));
                                        fputs(buf, file);
                                        break;
                                case '%':
                                        fputc('%', file);
                                        break;
                                case 'c':
                                        fputc((char)(va_arg(args, int)), file);
                                        break;
                                default:
                                        break;

                        }
                } else {
                        fputc(*ptr++, file);
                }
        }
}

void putc(char c) {
        fputc(c, stdout);
}

void puts(const char *str) {
        while(*str) {
                putc(*str);
                str++;
        }
}

void printf(const char *format, ...) {
        va_list ap;
        va_start(ap, format);
        vfprintf(stdout, format, ap);
        va_end(ap);
}
