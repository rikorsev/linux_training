obj-m := \
lktm_scull.o \
lktm_hello.o \
lktm_sync.o \
lktm_threads.o \
lktm_completions.o \
lktm_completions_2.o \
lktm_semaphores.o \
lktm_mutexes.o \
lktm_spinlocks.o \
lktm_rw_semaphores.o \
lktm_rw_spinlocks.o \
lktm_seqlocks.o \
lktm_rcu.o \
lktm_mem_kmalloc.o \
lktm_mem_lookaside_cache.o \
lktm_mem_get_free_pages.o \
lktm_mem_alloc_pages.o \
lktm_dev.o \
lktm_vled.o \
lktm_sysfs.o \
lktm_char_dev.o \
lktm_kobj.o

# Excluded from buidl for a while
#lktm_block.o \

MY_CFLAGS += -g -DDEBUG
ccflags += ${MY_CFLAGS}
cc += ${MY_CFLAGS}

all:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(CURDIR) modules
debug:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(CURDIR) modules EXTRA_CFLAGS="$(MY_CFLAGS)"
clean: clean_emacs_tmp
	rm -rf $(obj-m) $(obj-m:.o=.mod.c) $(obj-m:.o=.mod.o)
clean_all: clean
	rm -rf $(obj-m:.o=.ko) modules.order Module.symvers
clean_emacs_tmp:
	rm -rf *~
