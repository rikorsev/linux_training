obj-m := scull.o hello.o sync.o threads.o
MY_CFLAGS += -g -DDEBUG
ccflags += ${MY_CFLAGS}
cc += ${MY_CFLAGS}

KERNELDIR := ../linux-4.6.2

all:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(CURDIR) modules
debug:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(CURDIR) modules EXTRA_CFLAGS="$(MY_CFLAGS)"
clean:
	rm -rf $(obj-m) $(obj-m:.o=.mod.c) $(obj-m:.o=.mod.o)
