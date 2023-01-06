#pragma once
#include <cdefs.h>
#include <stddef.h>

size_t strlen(const char *str);
int strcmp(const char *s1, const char *s2);
char *strtok(char *s, char d);

void itoa (char *buf, int base, long long int d);
long long int atoi(char *str);
