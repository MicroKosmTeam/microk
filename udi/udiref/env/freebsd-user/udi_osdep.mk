#
# /usr/local/include is for libelf.
#
FLAGS=-pthread -Wall -I/usr/local/include

#
# /usr/local/lib is for libelf.
#
LIBS=-L/usr/local/lib -lelf

# Use the UW one until we get a custom one for FreeBSD
TORTURE_RUN='../freebsd-user/torture_run'
