SUBDIRS := task

test: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ clean
	$(MAKE) -C $@ $(MAKECMDGOALS)

clean all: $(SUBDIRS)

.PHONY: subdirs $(SUBDIRS) clean all
