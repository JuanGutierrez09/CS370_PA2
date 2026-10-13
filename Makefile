CC := gcc
CFLAGS := -std=c23 -Wall -Wextra -Werror -g -O1 -Iinclude
SRC := src/rbtree.c src/pool.c
TSRC := tests/test_rbtree.c tests/fault_alloc.c
ISRC := tests/test_rbtree_internal.c
BIN := build/test_rbtree
FUZZBIN := build/fuzz
IBIN := build/test_rbtree_internal

all: $(BIN) $(FUZZBIN) $(IBIN)

$(BIN): $(SRC) $(TSRC) include/rbtree.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) $(TSRC) -o $@ -lpthread

$(FUZZBIN): $(SRC) tests/fuzz.c tests/fault_alloc.c include/rbtree.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) tests/fuzz.c tests/fault_alloc.c -o $@ -lpthread

# ISRC #includes rbtree.c directly (white-box access to node internals),
# so rbtree.c is not compiled/linked separately here.
$(IBIN): $(ISRC) $(SRC) tests/fault_alloc.c include/rbtree.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(ISRC) src/pool.c tests/fault_alloc.c -o $@ -lpthread

test: $(BIN) $(FUZZBIN) $(IBIN)
	./$(BIN) && ./$(FUZZBIN) 100000 && ./$(IBIN)

asan: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
asan: clean test

memcheck: all
	valgrind --leak-check=full --show-leak-kinds=all \
		--error-exitcode=1 ./$(BIN)
	valgrind --leak-check=full --show-leak-kinds=all \
		--error-exitcode=1 ./$(FUZZBIN) 20000
	valgrind --leak-check=full --show-leak-kinds=all \
		--error-exitcode=1 ./$(IBIN)

clean:
	rm -rf build

.PHONY: all test asan memcheck clean