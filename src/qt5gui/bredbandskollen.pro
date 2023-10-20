QT          +=  webenginewidgets
TARGET       =  bredbandskollen
TEMPLATE     =  app
QMAKE_CXXFLAGS += -std=c++11 -DBBK_WEBVIEW

# To enable logging:
QMAKE_CXXFLAGS += -DTASKRUNNER_LOGERR
QMAKE_CXXFLAGS += -DTASKRUNNER_LOGWARN
QMAKE_CXXFLAGS += -DTASKRUNNER_LOGINFO
# QMAKE_CXXFLAGS += -DTASKRUNNER_LOGDBG

SOURCES += main.cpp \
        ../cli/utils.cpp \
        ../measurement/defs.cpp \
        ../framework/logger.cpp \
	../framework/socket.cpp \
	../framework/engine.cpp \
	../framework/task.cpp \
	../framework/taskconfig.cpp \
	../framework/bridgetask.cpp \
	../framework/socketconnection.cpp \
	../framework/serversocket.cpp \
	../framework/socketreceiver.cpp \
	../framework/eventloop.cpp \
	../http/httptask.cpp \
	../http/httpconnection.cpp \
        ../http/cookiemanager.cpp \
        ../http/cookiefile.cpp \
        ../http/websocketbridge.cpp \
        ../http/sha1.cpp \
        ../http/httpclienttask.cpp \
        ../http/webservertask.cpp \
        ../http/http_common.cpp \
        ../http/httpclientconnection.cpp \
        ../http/httpserverconnection.cpp \
        ../http/singlerequest.cpp \
	../json11/json11.cpp \
	../measurement/measurementtask.cpp \
	../measurement/singlerequesttask.cpp \
	../measurement/infotask.cpp \
	../measurement/latencytask.cpp \
	../measurement/rpingtask.cpp \
	../measurement/pingsweeptask.cpp \
	../measurement/warmuptask.cpp \
	../measurement/tickettask.cpp \
	../measurement/progresstask.cpp \
	../measurement/downloadtask.cpp \
	../measurement/uploadtask.cpp \
	../measurement/measurementagent.cpp \
	../measurement/speedtest.cpp \
	../measurement/uploadinfotask.cpp

HEADERS += ../cli/utils.h \
        ../measurement/defs.h \
        ../framework/logger.h \
	../framework/socket.h \
	../framework/engine.h \
	../framework/task.h \
	../framework/taskconfig.h \
	../framework/bridgetask.h \
	../framework/socketconnection.h \
	../framework/serversocket.h \
	../framework/eventloop.h \
	../http/httptask.h \
	../http/httpconnection.h \
	../http/cookiemanager.h \
        ../http/cookiefile.h \
        ../http/websocketbridge.h \
        ../http/sha1.h \
        ../http/httpclienttask.h \
        ../http/webservertask.h \
        ../http/http_common.h \
        ../http/httpclientconnection.h \
        ../http/httpserverconnection.h \
	../json11/json11.hpp \
	../measurement/measurementtask.h \
	../measurement/singlerequesttask.h \
	../measurement/infotask.h \
	../measurement/latencytask.h \
	../measurement/pingsweeptask.h \
	../measurement/rpingtask.h \
	../measurement/warmuptask.h \
	../measurement/tickettask.h \
	../measurement/progresstask.h \
	../measurement/downloadtask.h \
	../measurement/uploadtask.h \
	../measurement/measurementagent.h \
	../measurement/speedtest.h \
	../measurement/uploadinfotask.h
