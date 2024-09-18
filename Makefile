TARGET_MODULE := Random_Numbers_Module

obj-m += $(TARGET_MODULE).o
Random_Numbers_Module-objs := module.o ff.o

ccflags-y := -std=gnu99

KDIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
