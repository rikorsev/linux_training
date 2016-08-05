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
lktm_rcu.o

MY_CFLAGS += -g -DDEBUG
ccflags += ${MY_CFLAGS}
cc += ${MY_CFLAGS}

#KERNELDIR := ../linux-4.6.2

all:
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(CURDIR) modules
debug:
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(CURDIR) modules EXTRA_CFLAGS="$(MY_CFLAGS)"
clean: clean_emacs_tmp
	rm -rf $(obj-m) $(obj-m:.o=.mod.c) $(obj-m:.o=.mod.o)
clean_all: clean
	rm -rf $(obj-m:.o=.ko) modules.order Module.symvers
clean_emacs_tmp:
	rm -rf *~
