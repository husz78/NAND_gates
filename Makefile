CFLAGS = -Wall -Wextra -Wno-implicit-fallthrough -std=gnu17 -fPIC -O2
LDFLAGS = -shared -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc\
-Wl,--wrap=reallocarray -Wl,--wrap=free -Wl,--wrap=strdup -Wl,--wrap=strndup
OBJ = nand.o structs.o memory_tests.o
CC = gcc

.PHONY: all clean 

all: $(OBJ) libnand.so main testy

libnand.so: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

main: nand_example.o libnand.so
	$(CC) -L. -o $@ $< -lnand

testy: testy.o libnand.so
	$(CC) -L. -g -o $@ $< -lnand

nand.o: nand.h structs.h
structs.o: structs.h
memory_tests.o: memory_tests.h
nand_example.o: memory_tests.h nand.h
testy.o: nand.h

clean:
	rm -f *.o *.so ./main ./testy
