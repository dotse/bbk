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
INCL		= 

# Use this flag when compiling under Solaris
#EXTRADEFINES	= -DUNIX -DSOLARIS
#EXTRADEFINES	= -DUNIX -DOPENBSD
EXTRADEFINES	= -DUNIX -DLINUX

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


#SYSLIBS		= 
LIBS		= ${LOCLIBS} ${SYSLIBS}
OBJS		= tptestmaster.o newstuff.o tpcommon.o
SRCS		= tptestmaster.c newstuff.c tpcommon.c
LIBSRCS		= 
ISRCS		= tpengine.h
ALLSRCS		= ${SRCS} ${ISRCS} ${LIBSRCS}


all: tptestmaster

tptestmaster: ${OBJS}
	${CC} ${LDFLAGS} -o tptestmaster ${OBJS} ${LIBDIR} ${LIBS}

clean:
	rm -f *~ *.o *.BAK *.bak *.CKP core a.out
	rm -f tptestmaster

install:
	install -c -s -o bin -g bin -m 555 tptestmaster /usr/local/bin

depend: 
	${MAKEDEPEND} ${INCL} ${SRCS} ${LIBSRCS}

# DO NOT DELETE
