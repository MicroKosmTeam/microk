#include <ctype.h>

#define TRUE 1
#define FALSE 0
#define EOF 0

int isalnum(int ch) {
	if (isalpha(ch) || isdigit(ch))
		return TRUE;
	return FALSE;
}

int isalpha(int ch) {
	if (isupper(ch) || islower(ch))
		return TRUE;
	return FALSE;
}

int isblank(int ch) {
	if ((ch == '\t' || ch == ' ') && isspace(ch))
		return TRUE;
	return FALSE;
}

int iscntrl(int ch) {
	if ((ch >= 0x00 && ch <= 0x1f) || ch == 0x7f)
		return TRUE;
	return FALSE;
}

int isdigit(int ch) {
	if (ch == 0 ||
	    ch == 1 ||
	    ch == 2 ||
	    ch == 3 ||
	    ch == 4 ||
	    ch == 5 ||
	    ch == 6 ||
	    ch == 7 ||
	    ch == 8 ||
	    ch == 9)
		return TRUE;
	return FALSE;
}

int isgraph(int ch) {
	if (ch != ' ' && isprint(ch))
		return TRUE;
	return FALSE;
}

int islower(int ch) {
	if ((ch == 'a' ||
	     ch == 'b' ||
	     ch == 'c' ||
	     ch == 'd' ||
	     ch == 'e' ||
	     ch == 'f' ||
	     ch == 'g' ||
	     ch == 'h' ||
	     ch == 'i' ||
	     ch == 'j' ||
	     ch == 'k' ||
	     ch == 'l' ||
	     ch == 'm' ||
	     ch == 'n' ||
	     ch == 'o' ||
	     ch == 'p' ||
	     ch == 'q' ||
	     ch == 'r' ||
	     ch == 's' ||
	     ch == 't' ||
	     ch == 'u' ||
	     ch == 'v' ||
	     ch == 'w' ||
	     ch == 'x' ||
	     ch == 'y' ||
	     ch == 'z') &&
	     !(iscntrl(ch) ||
	       isdigit(ch) ||
	       ispunct(ch) ||
	       isspace(ch)))
		return TRUE;
	return FALSE;
}

int isprint(int ch) {
	if (ch > 0x1f && ch != 0x7f)
		return TRUE;
	return FALSE;
}

int ispunct(int ch) {
	if (isgraph(ch) && !isalnum(ch))
		return TRUE;
	return FALSE;
}

int isspace(int ch) {
	if((ch == ' ' || ch == '\t' || ch == '\n' ||
	    ch == '\v' || ch == '\f' || ch == '\r') && !isalnum(ch))
		return TRUE;
	return FALSE;
}

int isupper(int ch) {
	if ((ch == 'A' ||
	     ch == 'B' ||
	     ch == 'C' ||
	     ch == 'D' ||
	     ch == 'E' ||
	     ch == 'F' ||
	     ch == 'G' ||
	     ch == 'H' ||
	     ch == 'I' ||
	     ch == 'J' ||
	     ch == 'K' ||
	     ch == 'L' ||
	     ch == 'M' ||
	     ch == 'N' ||
	     ch == 'O' ||
	     ch == 'P' ||
	     ch == 'Q' ||
	     ch == 'R' ||
	     ch == 'S' ||
	     ch == 'T' ||
	     ch == 'U' ||
	     ch == 'V' ||
	     ch == 'W' ||
	     ch == 'X' ||
	     ch == 'Y' ||
	     ch == 'Z') &&
	     !(iscntrl(ch) ||
	       isdigit(ch) ||
	       ispunct(ch) ||
	       isspace(ch)))
		return TRUE;
	return FALSE;
}

int isxdigit(int ch) {
	if (ch == 0 ||
	    ch == 1 ||
	    ch == 2 ||
	    ch == 3 ||
	    ch == 4 ||
	    ch == 5 ||
	    ch == 6 ||
	    ch == 7 ||
	    ch == 8 ||
	    ch == 9 ||
	    ch == 'A' ||
	    ch == 'B' ||
	    ch == 'C' ||
	    ch == 'D' ||
	    ch == 'E' ||
	    ch == 'F' ||
	    ch == 'a' ||
	    ch == 'b' ||
	    ch == 'c' ||
	    ch == 'd' ||
	    ch == 'e' ||
	    ch == 'f')
		return TRUE;
	return FALSE;
}

int tolower(int ch) {
	if (ch == EOF) return EOF;

	if (isalpha(ch)) {
		if (islower(ch)) return ch;
		ch -= 32;
	}

	return ch;
}

int toupper(int ch) {
	if (ch == EOF) return EOF;

	if (isalpha(ch)) {
		if (isupper(ch)) return ch;
		ch += 32;
	}

	return ch;
}
