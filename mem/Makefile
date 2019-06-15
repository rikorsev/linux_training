obj-m := lktm_mem_alloc_pages.o lktm_mem_get_free_pages.o lktm_mem_kmalloc.o lktm_mem_lookaside_cache.o

MY_CFLAGS += -g -DDEBUG

all:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(CURDIR) modules
debug:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(CURDIR) modules EXTRA_CFLAGS="$(MY_CFLAGS)"
clean:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(CURDIR) clean
