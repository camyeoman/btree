CC=gcc
CFLAGS=-O0 -Werror=vla -std=gnu11 -g -fsanitize=address -pthread -lrt -lm
PERFFLAGS=-O0 -march=native -Werror=vla -std=gnu11 -pthread -lrt -lm
TESTFLAGS=-O0 -Werror=vla -std=gnu11 -g -fprofile-arcs -ftest-coverage -fsanitize=address -pthread -lrt -lm
NAME=btreestore
OBJECT=lib$(NAME).o
LIBRARY=lib$(NAME).a

project: btreestore.c btree.c
	mkdir -p bin obj

correctness: project btreestore.c
	$(CC) -c $(CFLAGS) btreestore.c -o obj/btreestore.o
	$(CC) -c $(CFLAGS) btree.c      -o obj/btree.o
	ar rcs $(LIBRARY) obj/*

performance: project btree.c btreestore.c
	$(CC) -c $(PERFFLAGS) btreestore.c -o obj/btreestore.o
	$(CC) -c $(PERFFLAGS) btree.c      -o obj/btree.o
	ar rcs $(LIBRARY) obj/*

tests: project btreestore.c btree.c
	$(CC) -c $(TESTFLAGS) btreestore.c -o obj/btreestore.o
	$(CC) -c $(TESTFLAGS) btree.c      -o obj/btree.o
	ar rcs $(LIBRARY) obj/*

run_tests: project correctness
	gcc -o bin/tests $(TESTFLAGS) tests/*.c -L. -lbtreestore
	bin/tests

clean:
	rm -rf bin obj
	rm -f *.gc{da,no}
	rm -f *.a
