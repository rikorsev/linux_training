obj-m := lktm_threads.o

MY_CFLAGS += -g -DDEBUG

all:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(CURDIR) modules
debug:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(CURDIR) modules EXTRA_CFLAGS="$(MY_CFLAGS)"
clean:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(CURDIR) clean
