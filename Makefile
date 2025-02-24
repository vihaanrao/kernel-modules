obj-m += mp1.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules -Wall
	gcc -o userapp userapp.c -Wall
	gcc -o userapp2 userapp2.c -Wall

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	$(RM) userapp
