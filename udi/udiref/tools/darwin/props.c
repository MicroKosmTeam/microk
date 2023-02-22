char *a __attribute__ ((section ("__text.__cstring"))) = "Hello";
char *b __attribute__ ((section ("__text.__cstring"))) = "World";
char *geta() {
	return a;
}
char *getb() {
	return b;
}
