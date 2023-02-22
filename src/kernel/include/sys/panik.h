#pragma once
#include <cdefs.h>

#define PANIK( msg ) panik( msg, __FILE__, __FUNCTION__, __LINE__ )

__attribute__((noreturn))
void panik(const char*, const char*, const char*, unsigned int);

