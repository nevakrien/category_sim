// test_category.c

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "category.h"

#define N 10000

typedef struct {
	ID id;
	int expected_magic;
} ID_Tracker;

static inline int is_valid_id(Category* cat, ID id) {
	if (id.slot >= cat->elements.len) return 0;
	return cat->elements.data[id.slot].id.global_id == id.global_id;
}

void test_category_safety() {
	Category cat = {0};
	ID_Tracker live_ids[N] = {0};
	int live_count = 0;

	srand((unsigned int)time(NULL));

	// Allocate N elements
	for (int i = 0; i < N; ++i) {
		Element* e = allocate_elem(&cat);
		int magic = i ^ 0xABCD1234;
		e->type = (const Type*)(uintptr_t)magic;
		live_ids[live_count++] = (ID_Tracker){ e->id, magic };
	}

	// Randomly delete ~half
	for (int i = 0; i < live_count; ++i) {
		if (rand() % 2 == 0) {
			delete_elem(&cat, live_ids[i].id);
			live_ids[i].id.global_id = -1;
		}
	}

	// Allocate another N/2
	for (int i = 0; i < N / 2; ++i) {
		Element* e = allocate_elem(&cat);
		e->type = (const Type*)(uintptr_t)0xDEADCAFE;
	}

	// Validate all still-live IDs
	for (int i = 0; i < live_count; ++i) {
		ID id = live_ids[i].id;
		if (id.global_id == -1) continue;

		if (!is_valid_id(&cat, id)) {
			fprintf(stderr, "Error: ID at slot %u is no longer valid!\n", id.slot);
			exit(1);
		}

		Element* e = &cat.elements.data[id.slot];
		int stored = (int)(uintptr_t)e->type;
		if (stored != live_ids[i].expected_magic) {
			fprintf(stderr, "Memory corruption! Expected %x, got %x at slot %u\n",
				live_ids[i].expected_magic, stored, id.slot);
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
