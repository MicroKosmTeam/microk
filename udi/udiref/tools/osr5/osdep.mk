CC=cc
CFLAGS=-Xa $(LOCALCFLAGS)
LD=ld
LDFLAGS=-r -o
LIB_YACC=-ly
LIB_LEX=-ll
LIB_OBJ=-lelf
GEN_MSG_CAT=gencat -X
OS_UDIBUILD_OBJS=osr5/obj_osdep.o osr5/link.o

osr5/obj_osdep.o: osr5/obj_osdep.c osr5/obj_osdep.h
	$(CC) -c $(CFLAGS) -o $@ osr5/obj_osdep.c

osr5/link.o: osr5/link.c osr5/osdep.h common/global.h common/common_api.h
	$(CC) -c $(CFLAGS) -o $@ osr5/link.c
