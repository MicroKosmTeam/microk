CC=`$(SHELL) solaris/whichcc`
#CC=gcc -fno-builtin
#CFLAGS=-Wall $(LOCALCFLAGS)
LOCCFLAGS=`$(SHELL) ../env/solaris/getarchflags -targetccopts`
CFLAGS=$(LOCFLAGS) $(LOCALCFLAGS) -D_REENTRANT
LDFLAGS=-r -o
LIB_YACC=-ly
LIB_LEX=-ll
LIB_OBJ=-lelf
GEN_MSG_CAT=gencat
OS_UDIBUILD_OBJS=solaris/link.o

$(OS_UDIBUILD_OBJS): solaris/link.c solaris/osdep.h common/global.h common/common_api.h
	$(CC) -c $(CFLAGS) -o $@ $<
