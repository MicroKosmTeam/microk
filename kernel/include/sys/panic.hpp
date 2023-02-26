#pragma once

#define PANIC( msg ) Panic( msg, __FILE__, __FUNCTION__, __LINE__ )

__attribute__((noreturn))
void Panic(const char *message, const char *file, const char *function, unsigned int line);
