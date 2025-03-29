#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "category.h"

#define N 10000

typedef struct {
	ID id;
	int expected_magic;
	int alive;
} ID_Tracker;

static inline int is_valid_id(Category* cat, ID id) {
	if (id.slot >= (int32_t)cat->elements.len) return 0;
	return cat->elements.data[id.slot].id.global_id == id.global_id;
}

void test_category_safety() {
	Category cat = {0};
	ID_Tracker ids[N] = {0};

	srand((unsigned int)time(NULL));

	// Allocate N elements
	for (int i = 0; i < N; ++i) {
		Element* e = allocate_elem(&cat);
		int magic = i ^ 0xABCD1234;
		e->type = (const Type*)(uintptr_t)magic;
		ids[i].id = e->id;
		ids[i].expected_magic = magic;
		ids[i].alive = 1;
	}

	// Randomly delete ~half
	for (int i = 0; i < N; ++i) {
		if (rand() % 2 == 0) {
			delete_elem(&cat, ids[i].id);
			ids[i].alive = 0;
		}
	}

	// Allocate another N/2
	for (int i = 0; i < N / 2; ++i) {
		Element* e = allocate_elem(&cat);
		e->type = (const Type*)(uintptr_t)0xDEADCAFE;
	}

	// Validate all still-live IDs
	for (int i = 0; i < N; ++i) {
		if (!ids[i].alive) continue;

		ID id = ids[i].id;

		if (!is_valid_id(&cat, id)) {
			fprintf(stderr, "Error: ID at slot %u is no longer valid!\n", id.slot);
			exit(1);
		}

		Element* e = &cat.elements.data[id.slot];
		int stored = (int)(uintptr_t)e->type;
		if (stored != ids[i].expected_magic) {
			fprintf(stderr, "Memory corruption! Expected %x, got %x at slot %u\n",
				ids[i].expected_magic, stored, id.slot);
			exit(1);
		}
	}

	free_category(&cat);
	printf("All tests passed ✅\n");
}

int main() {
	test_category_safety();
	return 0;
}
