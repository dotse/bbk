ALL += $(TARGET) $(PROGRAMS)

all: $(ALL)

OS:=$(shell uname)

CXXFLAGS += -O2 -W -Wall -I$(DIRLEVEL)

GLIB_COMPILE_RESOURCES = glib-compile-resources

ifeq ($(OS),Linux)
CXX = g++
endif

ifeq ($(OS),Darwin)
CXX = c++
CXXFLAGS += -x objective-c++
LIBS += -framework Foundation -framework ApplicationServices
endif

ifeq ($(OS),OpenBSD)
#CXX = ec++
CXX = clang++
endif

ifeq ($(OS),FreeBSD)
CXX = c++
endif

ifeq ($(OS),NetBSD)
CXX = clang++
CXXFLAGS += -I/usr/pkg/include
LIBS += -Wl,-R/usr/pkg/lib -L/usr/pkg/lib
endif

CXXFLAGS += -std=c++11

LINK ?= $(CXX)

ifeq ($(STATIC),1)
LDFLAGS += -static-libstdc++
endif

ifeq ($(STATIC),2)
LDFLAGS += -static -static-libstdc++
endif

ifneq ($(findstring clang,$(CXX)),)
CXXFLAGS += -Weverything -Wno-c++98-compat -Wno-exit-time-destructors \
	 -Wno-global-constructors -Wno-padded -Wno-disabled-macro-expansion \
	 -Wno-float-equal
endif

ifeq ($(CXX),g++)
CXXFLAGS += --pedantic -Wextra
endif

ifeq ($(GNUTLS),1)
CXXFLAGS += -DUSE_GNUTLS -I/usr/local/include
LIBS += -L/usr/local/lib -lgnutls
endif
ifeq ($(THREADS),1)
CXXFLAGS += -DUSE_THREADS
LIBS += -lpthread
endif
ifeq ($(LOGLEVEL),)
LOGLEVEL=info
endif
ifeq ($(LOGLEVEL),err)
CXXFLAGS += -DTASKRUNNER_LOGERR
endif
ifeq ($(LOGLEVEL),warn)
CXXFLAGS += -DTASKRUNNER_LOGWARN -DTASKRUNNER_LOGERR
endif
ifeq ($(LOGLEVEL),info)
CXXFLAGS += -DTASKRUNNER_LOGERR -DTASKRUNNER_LOGWARN -DTASKRUNNER_LOGINFO
endif
ifeq ($(LOGLEVEL),dbg)
CXXFLAGS += -DTASKRUNNER_LOGERR -DTASKRUNNER_LOGWARN
CXXFLAGS += -DTASKRUNNER_LOGINFO -DTASKRUNNER_LOGDBG
endif
ifeq ($(SANDBOXED),1)
CXXFLAGS += -DIS_SANDBOXED
endif

SOURCES += \
	$(DIRLEVEL)/framework/task.cpp \
	$(DIRLEVEL)/framework/taskconfig.cpp \
	$(DIRLEVEL)/framework/engine.cpp \
	$(DIRLEVEL)/framework/eventloop.cpp \
	$(DIRLEVEL)/framework/socket.cpp \
	$(DIRLEVEL)/framework/socketconnection.cpp \
	$(DIRLEVEL)/framework/serversocket.cpp \
	$(DIRLEVEL)/framework/socketreceiver.cpp \
	$(DIRLEVEL)/framework/logger.cpp \
	$(DIRLEVEL)/framework/bridgetask.cpp \
	$(DIRLEVEL)/framework/synchronousbridge.cpp

OPT_SOURCES += \
	$(DIRLEVEL)/framework/shortmessageconnection.cpp \
	$(DIRLEVEL)/framework/threadbridge.cpp \
	$(DIRLEVEL)/framework/unixdomainbridge.cpp \
	$(DIRLEVEL)/framework/unixdomainclient.cpp \
	$(DIRLEVEL)/framework/loadbalancer.cpp

OBJ=$(SOURCES:.cpp=.o)
EXTRA_OBJ=$(EXTRA_SOURCES:.cpp=.o)
OPT_OBJ=$(OPT_SOURCES:.cpp=.o)
RC_OBJ=$(RC_SOURCES:.rc.xml=.rc.o)
RC_DEPS=$(RC_SOURCES:.rc.xml=.rc_d)
RC_DEPS+=$(RC_SOURCES:.rc.xml=.rc.d)
RC_INTERMEDIATE+=$(RC_SOURCES:.rc.xml=.rc.cpp)

%.d: %.cpp
	$(CXX) -MM $(CXXFLAGS) -MT $(@:.d=.o) $< > $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.rc_d: %.rc.xml
	$(GLIB_COMPILE_RESOURCES) --generate-dependencies --dependency-file=$@ $<

%.rc.cpp: %.rc.xml
	$(GLIB_COMPILE_RESOURCES) --generate-source $< --target=$@

$(TARGET): $(OBJ) $(RC_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

$(PROGRAMS):
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

ifneq ($(MAKECMDGOALS),clean)
include $(SOURCES:.cpp=.d) $(RC_DEPS) $(EXTRA_SOURCES:.cpp=.d)
endif

clean:
	$(RM) $(CLEAN) $(TARGET) $(OBJ) $(OPT_OBJ) $(EXTRA_OBJ) *~ \
	  $(SOURCES:.cpp=.d) $(EXTRA_SOURCES:.cpp=.d) \
	  $(RC_OBJ) $(RC_DEPS) $(RC_INTERMEDIATE) $(PROGRAMS)
