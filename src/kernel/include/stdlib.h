#pragma once

#ifndef PREFIX
        #define RENAME(f)
#else
        #define RENAME(f) PREFIX ## f
#endif
