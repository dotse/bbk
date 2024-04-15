TARGET = runtest
DIRLEVEL = ../..
LOGLEVEL = dbg
GNUTLS = 0
THREADS = 0
CXXFLAGS += -g
LIBS += -lgtest -lgtest_main

SOURCES = task.cpp

include $(DIRLEVEL)/framework/mk.inc

test: all
	./runtest

clean: localclean

localclean:
	$(RM) log.txt

.PHONY: test localclean
