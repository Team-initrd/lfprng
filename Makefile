obj-m = lfprng.o
KVERSION = $(shell uname -r)

all: lfprng.o
#        make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

.PHONY: clean

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) clean

lfprng.o : lfprng.c
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) modules

