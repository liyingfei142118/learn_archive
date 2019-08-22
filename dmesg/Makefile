ifneq ($(KERNELRELEASE),) 
obj-m:= mykmsg.o     
else 
    KERNELDIR =/lib/modules/$(shell uname -r)/build 
    PWD := $(shell pwd) 
default: 
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm ‐rf *.order *.symvers *.o *~ core .depend .*.cmd *.ko *.mod.c   
endif
