CFLAGS=-Xa -O -Ki486 -Kno_lu -W0,-Lb $(LOCALCFLAGS)
LDFLAGS=-r -o
LIB_YACC=-ly
LIB_LEX=-ll
LIB_OBJ=-lelf
GEN_MSG_CAT=gencat -X
OS_UDIBUILD_OBJS=uw/link.o

$(OS_UDIBUILD_OBJS): uw/link.c uw/osdep.h common/global.h common/common_api.h
	$(CC) -c $(CFLAGS) -o $@ $<

OS_COMMON_OBJS=uw/params.o

$(OS_UDIBUILD_OBJS): uw/link.c
	$(CC) -c $(CFLAGS) -o $@ $<
$(OS_COMMON_OBJS): uw/params.c
	$(CC) -c $(CFLAGS) -o $@ $<

