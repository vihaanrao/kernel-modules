obj-m += mp1.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules -Wall
	gcc -o userapp userapp.c -Wall

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	$(RM) userapp
