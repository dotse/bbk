ifeq ($(WEBROOT),1)
CXXFLAGS += -DUSE_WEBROOT
endif

SOURCES += \
	$(DIRLEVEL)/http/sha1.cpp \
	$(DIRLEVEL)/http/httptask.cpp \
	$(DIRLEVEL)/http/httpclienttask.cpp \
	$(DIRLEVEL)/http/singlerequest.cpp \
	$(DIRLEVEL)/http/webservertask.cpp \
	$(DIRLEVEL)/http/http_common.cpp \
	$(DIRLEVEL)/http/httpconnection.cpp \
	$(DIRLEVEL)/http/httpclientconnection.cpp \
	$(DIRLEVEL)/http/cookiemanager.cpp \
	$(DIRLEVEL)/http/httpserverconnection.cpp \
	$(DIRLEVEL)/http/httprequestengine.cpp \
	$(DIRLEVEL)/http/websocketbridge.cpp

OPT_SOURCES += \
	$(DIRLEVEL)/http/cookiefile.cpp

# Note: we don't add $(DIRLEVEL)/http/cookiefile.cpp by default
#       since it uses the file system, which not all clients may want.

include $(DIRLEVEL)/framework/mk.inc
