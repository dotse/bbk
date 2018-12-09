SOURCES += \
	$(DIRLEVEL)/measurement/defs.cpp \
	$(DIRLEVEL)/measurement/speedtest.cpp \
	$(DIRLEVEL)/measurement/measurementtask.cpp \
	$(DIRLEVEL)/measurement/singlerequesttask.cpp \
	$(DIRLEVEL)/measurement/rpingtask.cpp \
	$(DIRLEVEL)/measurement/latencytask.cpp \
	$(DIRLEVEL)/measurement/pingsweeptask.cpp \
	$(DIRLEVEL)/measurement/warmuptask.cpp \
	$(DIRLEVEL)/measurement/tickettask.cpp \
	$(DIRLEVEL)/measurement/infotask.cpp \
	$(DIRLEVEL)/measurement/progresstask.cpp \
	$(DIRLEVEL)/measurement/downloadtask.cpp \
	$(DIRLEVEL)/measurement/uploadtask.cpp \
	$(DIRLEVEL)/measurement/measurementagent.cpp \
	$(DIRLEVEL)/measurement/uploadinfotask.cpp

include $(DIRLEVEL)/http/mk.inc
