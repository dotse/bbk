CC		= gcc
#
#      Use this for Linux and other systems that have makedepend
#      instead of mkdep.
#
MAKEDEPEND	= makedepend
#
#      For Solaris the mkdep script in this directory can be used.
#
#MAKEDEPEND	= ./mkdep
#
#      Use this for Free/Net/OpenBSD.
#
#MAKEDEPEND	= mkdep
CDEBUG		= -g
INCL		= -I ../../../os-dep/unix -I ../../../engine

# Use this flag when compiling under Solaris
#EXTRADEFINES	= -DUNIX -DSOLARIS
#EXTRADEFINES	= -DUNIX -DOPENBSD
EXTRADEFINES	= -D__WIN32

#
#      Use this if your machine has problems with localtime().
#
#EXTRADEFINES	= -DUSE_GMTIME

CFLAGS		= ${CDEBUG} ${EXTRADEFINES} ${INCL} -Wall
LDFLAGS		= ${CDEBUG}
LIBDIR		= 
LOCLIBS		= 

#
#      Use this for solaris and other SVR4 versions.
#
#SYSLIBS	= -lsocket -lnsl


# Source locations
ENGINE		= ../../../engine
OSDEP		= ../../../os-dep/unix

#SYSLIBS		= 
LIBS		= ${LOCLIBS} ${SYSLIBS}
OBJS		= tpio_unix.o tpengine.o tpcommon.o client.o tpclient.o getopt.o
SRCS		= tpio_unix.c tpengine.c tpcommon.c client.c tpclient.c getopt.c
LIBSRCS		= 
ISRCS		= tpengine.h tpio.h tpio_unix.h server.h tpclient.h
ALLSRCS		= ${SRCS} ${ISRCS} ${LIBSRCS}

all: tptestclient

tptestclient: ${OBJS}
	${CC} ${LDFLAGS} -o tptestclient ${OBJS} ${LIBDIR} ${LIBS}

tpio_unix.c:	${OSDEP}/tpio_unix.c
	cp -f ${OSDEP}/tpio_unix.c .

tpengine.c:	${ENGINE}/tpengine.c
	cp -f ${ENGINE}/tpengine.c .

tpcommon.c:	${ENGINE}/tpcommon.c
	cp -f ${ENGINE}/tpcommon.c .

tpclient.c:	${ENGINE}/tpclient.c
	cp -f ${ENGINE}/tpclient.c .

clean:
	rm -f tptestclient core *.o *.BAK *.bak *.CKP a.out tpio_unix.c tpengine.c tpcommon.c tpclient.c

install:
	install -c -s -o bin -g bin -m 555 tptestclient /usr/local/bin

depend: 
	${MAKEDEPEND} ${INCL} ${SRCS} ${LIBSRCS}

# DO NOT DELETE
