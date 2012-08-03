
CFLAGS=-g -I. 
#CC=i686-pc-mingw32-gcc
CC=i586-mingw32msvc-gcc
OBJS=mbus.o mbus-protocol.o mbus-protocol-aux.o mbus-serial-windows.o   mbus-serial-scan.o

mbus-serial-scan.exe: $(OBJS)
	$(CC) -g -I. $(OBJS) -o mbus-serial-scan.exe

clean:
	rm *.o
###

copy :mbus-serial-scan.exe
	echo copy 
	cp mbus-serial-scan.exe /cygdrive/Y/mbus/mbus-serial-scan-new.exe


