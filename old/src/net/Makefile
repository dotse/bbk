
TARG = libtptest-net.a
CPP = g++
#CDEBUG = -g
LINKER = g++

LIBS = -lpthread -lcurl
# -L/Developer/SDKs/MacOSX10.5.sdk/usr/lib 

INCLUDES = -I/Users/noskill/tptest5/curl-7.20.0/include/curl
# -I/Developer/SDKs/MacOSX10.5.sdk/usr/include 

CPPFLAGS = $(INCLUDES) -DUNIX -DMACOSX -DSWEDISH -finput-charset=ISO-8859-1 -isysroot /Developer/SDKs/MacOSX10.5.sdk/ -mmacosx-version-min=10.5
LINKERFLAGS = $(LIBS) -isysroot /Developer/SDKs/MacOSX10.5.sdk/ -mmacosx-version-min=10.5

OBJ_FILES = 	Downloader.o batch.o curl_get.o \
		executor.o icmp_ping.o tpio_unix.o \
		tcp_ping.o tp_test.o \
		tpclient.o tpcommon.o tpengine.o tvutils.o
default : $(TARG)

$(TARG) : $(OBJ_FILES)
	ar -q $(TARG) $(OBJ_FILES)

clean :	
	rm *.o
	rm $(TARG)
