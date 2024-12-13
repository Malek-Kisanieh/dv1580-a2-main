# Compiler and flags
CC = gcc
CFLAGS = -Wall -pthread -fPIC
LIB_NAME = libmemory_manager.so

# Source and Object Files
MEM_SRC = memory_manager.c
MEM_OBJ = $(MEM_SRC:.c=.o)

# Default target
all: mmanager list test_mmanager test_list

# Rule to create the dynamic library
$(LIB_NAME): $(MEM_OBJ)
	$(CC) -shared -o $@ $(MEM_OBJ) -pthread

# Rule to compile memory manager source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build the memory manager
mmanager: $(LIB_NAME)

# Build the linked list (compiles linked list source)
list: linked_list.o

# Test target to build the memory manager test program
test_mmanager: $(LIB_NAME)
	$(CC) -o test_memory_manager test_memory_manager.c -L. -lmemory_manager -lm

# Test target to build the linked list test program
test_list: $(LIB_NAME) linked_list.o
	$(CC) -o test_linked_list linked_list.c test_linked_list.c -L. -lmemory_manager -lm

# Run all tests
run_tests: run_test_mmanager run_test_list

# Run test cases for the memory manager
run_test_mmanager:
	LD_LIBRARY_PATH=. ./test_memory_manager 0

# Run test cases for the linked list
run_test_list:
	LD_LIBRARY_PATH=. ./test_linked_list 0

# Clean target to clean up build files
clean:
	rm -f $(MEM_OBJ) $(LIB_NAME) test_memory_manager test_linked_list linked_list.o
