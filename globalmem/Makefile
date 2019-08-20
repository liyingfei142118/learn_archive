ifneq ($(KERNELRELEASE),) 
obj-m:= globalmem.o     
else 
    KERNELDIR =/lib/modules/$(shell uname -r)/build 
    PWD := $(shell pwd) 
default: 
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm ‐rf *.o *~ core .depend .*.cmd *.ko *.mod.c   
endif
