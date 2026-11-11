# Glane — Makefile du socle portable core/
#
# Cible par défaut : bibliothèque statique + binaires de test.
# `make test` exécute tous les tests et rend 0 si tout est vert.
# Règle de compilation (CONTRACT.md §0) : C11 strict, aucun avertissement.

CC       ?= gcc
AR       ?= ar
CFLAGS   ?= -std=c11 -O2 -Wall -Wextra -Werror
INCLUDE  := -Icore/include -Icore/tests
BUILD    := build

CORE_SRC := $(wildcard core/src/*.c)
CORE_OBJ := $(patsubst core/src/%.c,$(BUILD)/core/%.o,$(CORE_SRC))
TEST_SRC := $(wildcard core/tests/test_*.c)
TEST_BIN := $(patsubst core/tests/%.c,$(BUILD)/%,$(TEST_SRC))

.PHONY: all test asan sizecheck clean
all: $(BUILD)/libglane_core.a $(TEST_BIN)

$(BUILD)/core/%.o: core/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(BUILD)/libglane_core.a: $(CORE_OBJ)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(BUILD)/test_%: core/tests/test_%.c $(BUILD)/libglane_core.a
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) $< -L$(BUILD) -lglane_core -o $@

test: all
	@fail=0; for t in $(TEST_BIN); do \
		echo "== $$t =="; ./$$t || fail=1; \
	done; \
	if [ $$fail -ne 0 ]; then echo "TESTS FAILED"; exit 1; fi; \
	echo "ALL TESTS PASSED"

# Passe ASan/UBSan (CONTRACT.md §0) : reconstruit tout avec les sanitizers.
asan:
	@$(MAKE) clean
	@$(MAKE) all CFLAGS="-std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror"
	@fail=0; for t in $(TEST_BIN); do ./$$t || fail=1; done; \
	if [ $$fail -ne 0 ]; then echo "ASAN TESTS FAILED"; exit 1; fi; \
	echo "ASAN ALL TESTS PASSED"

sizecheck:
	python3 scripts/sizecheck.py

clean:
	rm -rf $(BUILD)