CC := gcc
CFLAGS := -std=c23 -Wall -Wextra -Werror -g -O1 -Iinclude
SRC := src/rbtree.c
TSRC := tests/test_rbtree.c
ISRC := tests/test_rbtree_internal.c
BIN := build/test_rbtree
FUZZBIN := build/fuzz
IBIN := build/test_rbtree_internal

all: $(BIN) $(FUZZBIN) $(IBIN)

$(BIN): $(SRC) $(TSRC) include/rbtree.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) $(TSRC) -o $@

$(FUZZBIN): $(SRC) tests/fuzz.c include/rbtree.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) tests/fuzz.c -o $@

# ISRC #includes rbtree.c directly (white-box access to node internals),
# so SRC is not compiled/linked separately here.
$(IBIN): $(ISRC) $(SRC) include/rbtree.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(ISRC) -o $@

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