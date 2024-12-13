CC = gcc
CFLAGS = -Wall -pthread -fPIC
LIB = -L. -lmemory_manager -lm
TARGETS = memory_manager.o libmemory_manager.so test_memory_manager linked_list

all: $(TARGETS)

memory_manager.o: memory_manager.c
	$(CC) $(CFLAGS) -c memory_manager.c -o memory_manager.o

libmemory_manager.so: memory_manager.o
	$(CC) -shared -o libmemory_manager.so memory_manager.o

linked_list.o: linked_list.c
	$(CC) $(CFLAGS) -c linked_list.c -o linked_list.o

linked_list: linked_list.o
	$(CC) linked_list.o memory_manager.o $(LIB) -o linked_list

test_memory_manager: test_memory_manager.c
	$(CC) $(CFLAGS) -o test_memory_manager test_memory_manager.c $(LIB)

run_tests: test_memory_manager
	LD_LIBRARY_PATH=. ./test_memory_manager $(TEST_FUNC)

clean:
	rm -f *.o *.so test_memory_manager linked_list
