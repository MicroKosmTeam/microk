#pragma once
#include <stdio.h>
#include <cdefs.h>

#define PANIK( msg ) panik( msg, __FILE__, __FUNCTION__, __LINE__ )

void panik(const char*, const char*, const char*, unsigned int);
