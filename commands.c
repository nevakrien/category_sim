#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "commands.h"

void free_undo_stack(UndoStack* undo_stack){
    while(undo_stack->stack.len){
        Command cmd = POP(&undo_stack->stack);
        cmd.destroy(cmd.data);
    }

    free(undo_stack->stack.data);
    memset(undo_stack,0,sizeof(UndoStack));
}


void apply_command(Category* cat, UndoStack* undo_stack, Command command) {
    bool shrunk = false;
    // Always truncate redo history
    ASSERT(undo_stack->len<=undo_stack->stack.len);
    while(undo_stack->stack.len!=undo_stack->len){
        shrunk = true;
        Command cmd = POP(&undo_stack->stack);
        cmd.destroy(cmd.data);
    }
    APPEND(&undo_stack->stack, command);
    if(shrunk){
        SHRINK(&undo_stack->stack);
    }
    undo_stack->len = undo_stack->stack.len;

    command.do_command(cat, command.data);
}

void undo_last(Category* cat, UndoStack* undo_stack) {
    ASSERT(undo_stack->len > 0);
    Command* cmd = &undo_stack->stack.data[--undo_stack->len];
    cmd->undo_command(cat, cmd->data);
}

void redo_last(Category* cat, UndoStack* undo_stack) {
    ASSERT(undo_stack->len < undo_stack->stack.len);
    Command* cmd = &undo_stack->stack.data[undo_stack->len++];
    cmd->do_command(cat, cmd->data);
}

void _COMMAND_add_element(Category* cat, void* data) {
    ID* id = data;
    Element* elem = allocate_elem(cat);
    *id = elem->id;
}

void _COMMAND_undo_add(Category* cat, void* data) {
    ID* id = data;
    delete_elem(cat, *id);
    ASSERT((int32_t)cat->global_id == id->global_id);
    cat->global_id--;
}


Command make_raw_add_command() {
    ID* data = null_check(malloc(sizeof(ID)));
    return (Command){
        .data = data,
        .do_command = _COMMAND_add_element,
        .undo_command = _COMMAND_undo_add,
        .destroy = free
    };
}


void _COMMAND_do_remove(Category* cat, void* data) {
    Element* backup = data;
    Element* elem = get_elem(cat,backup->id);
    ASSERT(elem);
    // ASSERT(!memcmp(elem->id == backup->id));
    *backup = *elem;
    delete_elem(cat, elem->id);
}

void _COMMAND_undo_remove(Category* cat, void* data) {
    Element* cmd = data;
    revive_elem(cat, *cmd);
}

Command make_raw_delete_command(ID id) {
    Element* data = null_check(malloc(sizeof(Element)));
    data->id = id;
    return (Command){
        .data = data,
        .do_command = _COMMAND_do_remove,
        .undo_command = _COMMAND_undo_remove,
        .destroy = free
    };
}
