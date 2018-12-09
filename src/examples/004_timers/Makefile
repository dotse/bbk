# Name of the executable program to be built:
TARGET = timers

# Relative path to where the framework directory is located:
DIRLEVEL = ../..

# Possible LOGLEVEL values: dbg, info, warn, err, none
LOGLEVEL = dbg

# Set to 1 to link with GnuTLS, i.e. to enable SSL support.
GNUTLS = 0

# All C++ source files used by the target program
SOURCES = main.cpp

# To be able to debug the target program
CXXFLAGS += -g

# Include the HTTP support files. It will also include the base framework,
# i.e. $(DIRLEVEL)/framework/mk.inc
# The below line should be the last one in the Makefile.
include $(DIRLEVEL)/http/mk.inc
