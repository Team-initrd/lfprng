obj-m = lfprng.o
KVERSION = $(shell uname -r)

all: lfprng.o threadread tester pthreadtest
#        make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

.PHONY: clean load all

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) clean
	-rm threadread tester pthreadtest

lfprng.o : lfprng.c
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) modules

load : lfprng.o
	-/sbin/rmmod lfprng
	/sbin/insmod lfprng.ko

threadread : threadread.c
	gcc -o threadread -lpthread threadread.c

tester : tester.c
	gcc -o tester -fopenmp tester.c


pthreadtest : pthreadtest.c
	gcc -o pthreadtest -lpthread pthreadtest.c

