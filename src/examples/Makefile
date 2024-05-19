# These can't be built in parallel since there's a race when creating
# the dependency files, so "make" / "make -j1" is needed.

SUBDIRS := $(wildcard [0-9][0-9][0-9]_*)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ clean # needed because of conflicting obj instrumentations
	$(MAKE) -C $@ -j    # internal parallel build is ok

clean:
	@$(foreach dir,$(SUBDIRS),$(MAKE) -C$(dir) clean;)

.PHONY: $(SUBDIRS) clean all
