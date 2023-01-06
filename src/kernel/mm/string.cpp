#include <mm/string.h>
#include <mm/heap.h>

size_t strlen(const char *str) {
        const char *s;
        for (s = str; *s; ++s);
        return (s - str);
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

// BEWARE: this can't be used before the heap is initialized
// That should never be a problem
char *strtok(char *s, char d) {
        // Stores the state of string
        static char *input = NULL;
 
        // Initialize the input string
        if (s != NULL)
        input = s;

        // Case for final token
        if (input == NULL)
        return NULL;
 
        // Stores the extracted string
        char* result = new char[strlen(input) + 1];
        int i = 0;
 
        // Start extracting string and
        // store it in array
        for (; input[i] != '\0'; i++) {

                // If delimiter is not reached
                // then add the current character
                // to result[i]
                if (input[i] != d)
                        result[i] = input[i];
 
                // Else store the string formed
                else {
                        result[i] = '\0';
                        input = input + i + 1;
                        return result;
                }
        }
 
        // Case when loop ends
        result[i] = '\0';
        input = NULL;

        // Return the resultant pointer
        // to the string
        return result;
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
