#ifndef COMMANDS_H
#define COMMANDS_H

#include "utils.h"
#include "category.h"


typedef struct {
    void* data;
    void (*do_command)(Category* cat, void* data);
    void (*undo_command)(Category* cat, void* data);
    void (*destroy)(void* data);
}Command;

DEFINE_VECTOR_TYPE(Command,CommandStack);

typedef struct {
	CommandStack stack;
	size_t len;
} UndoStack;

void free_undo_stack(UndoStack* undos);

Command make_raw_delete_command(ID id);
Command make_raw_add_command();

void apply_command(Category* cat,UndoStack* stack, Command command);
void undo_last(Category* cat,UndoStack* stack);
void redo_last(Category* cat,UndoStack* stack);
#endif //COMMANDS_H