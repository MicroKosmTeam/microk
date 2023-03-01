#include <mm/string.hpp>

size_t strlen(const char *str) {
        const char *s;
        for (s = str; *s; ++s);
        return (s - str);
}

char *strcpy(char *strDest, const char *strSrc){
        if(strDest==NULL || strSrc==NULL) return NULL;
        char *temp = strDest;
        while(*strDest++ = *strSrc++);
        return temp;
}

int strcmp(const char *s1, const char *s2) {
        const unsigned char *p1 = (const unsigned char *)s1;
        const unsigned char *p2 = (const unsigned char *)s2;

        while (*p1 != '\0') {
                if (*p2 == '\0') return  1;
                if (*p2 > *p1)   return -1;
                if (*p1 > *p2)   return  1;

                p1++;
                p2++;
        }

        if (*p2 != '\0') return -1;

        return 0;
}

bool is_delim(char c, char *delim) {
	while(*delim != '\0') {
		if(c == *delim)
			return true;
		delim++;
	}

	return false;
}

char *strtok(char *s, char *delim) {
	static char *p; // start of the next search
	if(!s) s = p;
	if(!s) return NULL;

	// handle beginning of the string containing delims
	while(true) {
		if(is_delim(*s, delim)) {
			s++;
			continue;
		}

		if(*s == '\0') {
			return NULL; // we've reached the end of the string
		}

		// now, we've hit a regular character. Let's exit the
		// loop, and we'd need to give the caller a string
		// that starts here.
		break;
	}

	char *ret = s;

	while(true) {
		if(*s == '\0') {
			p = s; // next exec will return NULL
			return ret;
		}

		if(is_delim(*s, delim)) {
			*s = '\0';
			p = s + 1;
			return ret;
		}

		s++;
	}
}


void itoa (char *buf, int base, long long int d) {
        char *p = buf;
        char *p1, *p2;
        unsigned long long ud = d;
        int divisor = 10;

        /*  If %d is specified and D is minus, put ‘-’ in the head. */
        if (base == 'd' && d < 0) {
                *p++ = '-';
                buf++;
                ud = -d;
        } else if (base == 'x')
                divisor = 16;

        /*  Divide UD by DIVISOR until UD == 0. */
        do {
                int remainder = ud % divisor;
                *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
        } while (ud /= divisor);

        /*  Terminate BUF. */
        *p = 0;

        /*  Reverse BUF. */
        p1 = buf;
        p2 = p - 1;

        while (p1 < p2) {
                char tmp = *p1;
                *p1 = *p2;
                *p2 = tmp;
                p1++;
                p2--;
        }
}

long long int atoi(char *str) {
        // Initialize result
        long long int res = 0;

        // Iterate through all characters
        // of input string and update result
        // take ASCII character of corresponding digit and
        // subtract the code from '0' to get numerical
        // value and multiply res by 10 to shuffle
        // digits left to update running total
        for (int i = 0; str[i] != '\0'; ++i)
                res = res * 10 + str[i] - '0';

        // return result.
        return res;
}
