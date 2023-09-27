TARGET = cli
DIRLEVEL = ..

# Possible LOGLEVEL values: dbg, info, warn, err, none
LOGLEVEL=info

# Uncomment if GnuTLS version 3.5 or later is available
# GNUTLS=1

SOURCES=../http/cookiefile.cpp \
	main.cpp \
	../measurement/wsdownloadtask.cpp \
	utils.cpp \
	cliclient.cpp

ifeq ($(SERVER),1)
SOURCES += ../server/measurementserver.cpp \
	../server/ticketclient.cpp
CXXFLAGS += -DRUN_SERVER
else
CLEAN += ../server/measurementserver.o ../server/ticketclient.o
endif

# CXXFLAGS += -m32 -I/usr/i686-linux-gnu/include/c++/8/i686-linux-gnu
# LDFLAGS += -m32

include $(DIRLEVEL)/measurement/mk.inc
