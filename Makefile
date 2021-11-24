obj-m += ryandriver.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	rm -f *.mod.*
	rm -f *.ko
	rm -f *.o
	rm -f -r .tmp_versions
	rm -f -r Module.symvers
	rm -f -r modules.order
