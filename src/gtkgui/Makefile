TARGET = bredbandskollen
DIRLEVEL = ..

# Possible LOGLEVEL values: dbg, info, warn, err, none
LOGLEVEL=info

# We will use Logger in more than one thread:
THREADS=1

# Uncomment if GnuTLS version 3.5 or later is available
# GNUTLS=1

SOURCES=../http/cookiefile.cpp \
	../framework/unixdomainbridge.cpp \
	../framework/unixdomainclient.cpp \
	../framework/shortmessageconnection.cpp \
	main.cpp \
	gtkclient.cpp \
	../cli/utils.cpp

CXXFLAGS=$(shell pkg-config --cflags gtk+-3.0)
LIBS=$(shell pkg-config --libs gtk+-3.0)

#CXXFLAGS += -g

include $(DIRLEVEL)/measurement/mk.inc
