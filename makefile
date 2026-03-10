# --- Compiler and Tool Definitions ---
CC = gcc
CFLAGS = -Wall -g
AR = ar
ARFLAGS = rcs

# --- Default Target ---
# Typing 'make' will build all of these automatically in the correct order
all: libksocket.a initksocket user1 user2

# =========================================================
# 1. Rule to generate the static library (libksocket.a)
# =========================================================
libksocket.a: ksocket.o
	$(AR) $(ARFLAGS) libksocket.a ksocket.o

ksocket.o: ksocket.c ksocket.h
	$(CC) $(CFLAGS) -c ksocket.c -o ksocket.o

# =========================================================
# 2. Rule to create the executable for initksocket.c
# =========================================================
# Depends on initksocket.c and the library being built first.
# Links the ksocket library (-lksocket) and pthreads (-lpthread).
initksocket: initksocket.c libksocket.a
	$(CC) $(CFLAGS) initksocket.c -L. -lksocket -lpthread -o initksocket

# =========================================================
# 3. Rules to create the executables for user1.c and user2.c
# =========================================================
user1: user1.c libksocket.a
	$(CC) $(CFLAGS) user1.c -L. -lksocket -lpthread -o user1

user2: user2.c libksocket.a
	$(CC) $(CFLAGS) user2.c -L. -lksocket -lpthread -o user2

# =========================================================
# Cleanup Rule
# =========================================================
# Typing 'make clean' will delete all generated files
clean:
	rm -f *.o *.a initksocket user1 user2