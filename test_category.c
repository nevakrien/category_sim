//test_category.c

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "category.h"
#include "commands.h"

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
	printf("Base tests passed ✅\n");
}

void test_command_stack() {
	Category cat = {0};
	CommandStack stack = {0};

	ID_Tracker ids[N] = {0};     // All IDs
	int alive_indices[N];        // Indexes of currently alive IDs
	int alive_count = 0;

	srand((unsigned int)time(NULL));

	for (int i = 0; i < N; ++i) {
		int do_add = (alive_count == 0 || rand() % 2 == 0);

		if (do_add) {
			Command cmd = {.type = CMD_ADD_ELEMENT};
			do_command(&cat, &cmd);
			APPEND(&stack, cmd);

			Element* e = &cat.elements.data[cmd.backup.id.slot];
			int magic = i ^ 0xA55A1234;
			e->type = (const Type*)(uintptr_t)magic;

			ids[i] = (ID_Tracker){
				.id = e->id,
				.expected_magic = magic,
				.alive = 1
			};
			alive_indices[alive_count++] = i;
		} else {
			int idx_in_alive = rand() % alive_count;
			int tracker_index = alive_indices[idx_in_alive];
			ID_Tracker* tracker = &ids[tracker_index];

			// Remove from alive list
			alive_indices[idx_in_alive] = alive_indices[--alive_count];

			Command cmd = {
				.type = CMD_DELETE_ELEMENT,
				.backup = cat.elements.data[tracker->id.slot]
			};
			do_command(&cat, &cmd);
			APPEND(&stack, cmd);

			tracker->alive = 0;
		}

		// Validate current state
		for (int j = 0; j < N; ++j) {
			if (!ids[j].alive) continue;

			if (!is_valid_id(&cat, ids[j].id)) {
				fprintf(stderr, "Invalid ID at slot %d during step %d\n", ids[j].id.slot, i);
				exit(1);
			}
			Element* e = &cat.elements.data[ids[j].id.slot];
			int magic = (int)(uintptr_t)e->type;
			if (magic != ids[j].expected_magic) {
				fprintf(stderr, "Corruption at slot %d: expected %x, got %x at step %d\n",
				        ids[j].id.slot, ids[j].expected_magic, magic, i);
				exit(1);
			}
		}
	}

	// Undo everything
	for (int i = stack.len - 1; i >= 0; --i) {
		undo_command(&cat, &stack.data[i]);
	}

	// Check that everything is clean
	for (uint32_t i = 0; i < cat.elements.len; ++i) {
		if (cat.elements.data[i].id.slot != -1 || cat.elements.data[i].id.global_id != -1) {
			fprintf(stderr, "Element at slot %d not cleared after undo\n", i);
			exit(1);
		}
	}

	free(stack.data);
	free_category(&cat);
	printf("Command stack test passed ✅\n");
}


int main() {
	srand((unsigned int)time(NULL));

	test_category_safety();
	test_command_stack();

	return 0;
}
