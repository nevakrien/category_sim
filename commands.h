#ifndef COMMANDS_H
#define COMMANDS_H

#include "utils.h"
#include "category.h"

// For now, we define a simple command enum and structure
typedef enum {
    CMD_ADD_ELEMENT,
    CMD_DELETE_ELEMENT
} CommandType;

typedef struct {
    CommandType type;
    Element backup; // used for undoing delete or revive
}Command;

DEFINE_VECTOR_TYPE(Command,CommandStack);

void do_command(Category* cat, Command* command);
void undo_command(Category* cat, Command* command);

#endif //COMMANDS_H