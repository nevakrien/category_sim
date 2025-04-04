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
		e->data = (void*)(uintptr_t)magic;
		ids[i].id = e->id;
		ids[i].expected_magic = magic;
		ids[i].alive = 1;
	}

	// Randomly delete ~half
	for (int i = 0; i < N; ++i) {
		if (rand() % 2 == 0 && ids[i].alive) {
			delete_elem(&cat, ids[i].id);
			ids[i].alive = 0;
		}
	}

	// Allocate another N/2
	for (int i = 0; i < N / 2; ++i) {
		Element* e = allocate_elem(&cat);
		e->data = (void*)(uintptr_t)0xDEADCAFE;
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
		int stored = (int)(uintptr_t)e->data;
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

	stack.capacity = N;
	stack.data = null_check(calloc(N,sizeof(Command)));

	ID_Tracker ids[N] = {0};     // All IDs
	int alive_indices[N];        // Indexes of currently alive IDs
	int alive_count = 0;

	for (int i = 0; i < N; ++i) {
		// srand(rand() ^ i);
		int do_add = (alive_count == 0 || rand() % 2 == 0);

		if (do_add) {
			Command cmd = make_raw_add_command();
			cmd.do_command(&cat, cmd.data);
			APPEND(&stack, cmd);

			Element* e = &cat.elements.data[((ID*)(cmd.data))->slot];
			int magic = i ^ 0xA55A1234;
			e->data = (void*)(uintptr_t)magic;

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

			Command cmd = make_raw_delete_command(tracker->id);
			cmd.do_command(&cat, cmd.data);
			APPEND(&stack, cmd);

			tracker->alive = 0;
		}

		// Command cmd = {.type = CMD_DUMP_ALL};
		// do_command(&cat, &cmd);
		// APPEND(&stack, cmd);		

		// Validate that all positive global_ids are unique
		{
			for (uint32_t i = 0; i < cat.elements.len; ++i) {
				Element* e = &cat.elements.data[i];
				int32_t gid = e->id.global_id;
				if (gid > 0) {
					for (uint32_t j = 0; j < i; ++j) {
						if (cat.elements.data[j].id.global_id == gid) {
							fprintf(stderr, "Duplicate global_id %d found at slots %u and %u\n",
							        gid, j, i);
							exit(1);
						}
					}
				}
			}
		}


		// Validate current state
		for (int j = 0; j < N; ++j) {
			if (!ids[j].alive) continue;

			if (!is_valid_id(&cat, ids[j].id)) {
				fprintf(stderr, "Invalid ID at slot %d during step %d\n", ids[j].id.slot, i);
				exit(1);
			}
			Element* e = &cat.elements.data[ids[j].id.slot];
			int magic = (int)(uintptr_t)e->data;
			if (magic != ids[j].expected_magic) {
				fprintf(stderr, "Corruption at slot %d: expected %x, got %x at step %d\n",
				        ids[j].id.slot, ids[j].expected_magic, magic, i);
				exit(1);
			}
		}
	}

	// Undo everything
	for (int i = stack.len - 1; i >= 0; --i) {
		Command cmd = stack.data[i];
		cmd.undo_command(&cat, cmd.data);
	}

	// Check that everything is clean
	for (uint32_t i = 0; i < cat.elements.len; ++i) {
		if (cat.elements.data[i].id.slot != -1 || cat.elements.data[i].id.global_id != -1) {
			fprintf(stderr, "Element at slot %d not cleared after undo\n", i);
			exit(1);
		}
	}

	// free(stack.data);
	free_undo_stack(&(UndoStack){.stack=stack});
	free_category(&cat);
	printf("Command stack test passed ✅\n");
	fflush(stdout);

}

static inline void check_valid_id_impl(Category* cat, ID id, const char* file, int line) {
	if (id.slot < 0 ||
	    (uint32_t)id.slot >= cat->elements.len ||
	    cat->elements.data[id.slot].id.global_id != id.global_id) {
		fprintf(stderr, "%s:%d: ERROR: ID (%d, %d) expected to be valid but isn't\n",
		        file, line, id.slot, id.global_id);
		exit(1);
	}
}

static inline void check_invalid_id_impl(Category* cat, ID id, const char* file, int line) {
	if (id.slot >= 0 && (uint32_t)id.slot < cat->elements.len) {
		Element* e = &cat->elements.data[id.slot];
		if (e->id.global_id == id.global_id) {
			fprintf(stderr, "%s:%d: ERROR: ID (%d, %d) expected to be invalid but is still alive\n",
			        file, line, id.slot, id.global_id);
			exit(1);
		}
	}
}

#define CHECK_VALID_ID(cat, id)   check_valid_id_impl((cat), (id), __FILE__, __LINE__)
#define CHECK_INVALID_ID(cat, id) check_invalid_id_impl((cat), (id), __FILE__, __LINE__)


void test_manual_command_sequence() {
	// printf("=== Starting manual command sequence test ===\n");

	Category cat = {0};
	UndoStack undo = {0};

	ID e0, e1, e2, e3;

	// Add 3 elements
	for (int i = 0; i < 3; ++i) {
		Command c = make_raw_add_command();
		apply_command(&cat, &undo, c);
		ID id = *(ID*)(undo.stack.data[undo.len - 1].data);
		if (i == 0) e0 = id;
		if (i == 1) e1 = id;
		if (i == 2) e2 = id;
		CHECK_VALID_ID(&cat, id);
	}

	// Undo last 2 adds (e2, e1)
	undo_last(&cat, &undo); CHECK_INVALID_ID(&cat, e2);
	undo_last(&cat, &undo); CHECK_INVALID_ID(&cat, e1);
	CHECK_VALID_ID(&cat, e0);

	// Redo one (e1)
	redo_last(&cat, &undo); CHECK_VALID_ID(&cat, e1);//here we break

	// Add a new one (e3), which truncates redo
	{
		Command c = make_raw_add_command();
		apply_command(&cat, &undo, c);
		e3 = *(ID*)(undo.stack.data[undo.len - 1].data);
		CHECK_VALID_ID(&cat, e3);

		if (undo.len < undo.stack.len) {
			fprintf(stderr, "ERROR: Redo tail not truncated after new add\n");
			exit(1);
		}
	}

	// Delete e0 explicitly
	{
		Command c = make_raw_delete_command(e0);
		apply_command(&cat, &undo, c);
		CHECK_INVALID_ID(&cat, e0);
	}

	// Undo that delete → e0 is back
	undo_last(&cat, &undo); CHECK_VALID_ID(&cat, e0);

	// Undo everything
	while (undo.len > 0) {
		undo_last(&cat, &undo);
	}

	CHECK_INVALID_ID(&cat, e0);
	CHECK_INVALID_ID(&cat, e1);
	CHECK_INVALID_ID(&cat, e2);
	CHECK_INVALID_ID(&cat, e3);

	free_undo_stack(&undo);
	free_category(&cat);

	printf("Manual command sequence test passed ✅\n");
}


int main() {
	srand((unsigned int)time(NULL));

	test_category_safety();
	test_command_stack();
	test_manual_command_sequence();

	return 0;
}
