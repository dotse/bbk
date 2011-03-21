TARG = libtptest-graph.a
LINKER = g++
CC = g++
#CDEBUG = -g

LIBS = 		-lpthread -lgd


INCLUDES = 	-I/Users/noskill/tptest5/gd-2.0.35


CPPFLAGS = $(INCLUDES) -DUNIX -DMACOSX -isysroot /Developer/SDKs/MacOSX10.5.sdk/ -mmacosx-version-min=10.5
#-finput-charset=ISO-8859-1 

LINKERFLAGS = $(LIBS) -isysroot /Developer/SDKs/MacOSX10.5.sdk/ -mmacosx-version-min=10.5

OBJ_FILES = graphstruct.o tvutils.o

default : $(TARG)

$(TARG) : $(OBJ_FILES)
	ar -q $(TARG) $(OBJ_FILES)

clean :	
	rm *.o ${TARG}