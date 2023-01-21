#pragma once

#define EOF 1;

#ifndef PREFIX
        #define RENAME(f)
#else
        #define RENAME(f) PREFIX ## f
#endif
