CC = gcc
CFLAGS = -Wall -g
AR = ar
ARFLAGS = rcs

all: libksocket.a initksocket user1 user2

libksocket.a: ksocket.o
	$(AR) $(ARFLAGS) libksocket.a ksocket.o

ksocket.o: ksocket.c ksocket.h
	$(CC) $(CFLAGS) -c ksocket.c -o ksocket.o

initksocket: initksocket.c libksocket.a
	$(CC) $(CFLAGS) initksocket.c -L. -lksocket -lpthread -o initksocket

user1: user1.c libksocket.a
	$(CC) $(CFLAGS) user1.c -L. -lksocket -lpthread -o user1

user2: user2.c libksocket.a
	$(CC) $(CFLAGS) user2.c -L. -lksocket -lpthread -o user2

clean:
	rm -f *.o *.a initksocket user1 user2