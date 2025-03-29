#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "commands.h"

bool needs_redo(CommandType type){
    switch (type){
        case CMD_ADD_ELEMENT:
        case CMD_DELETE_ELEMENT:
            return true;
        case CMD_DUMP_ALL:
            return false;
    };

    return true;//never reached
}

void apply_command(Category* cat, UndoStack* undo_stack, Command command){
    if(needs_redo(command.type)){
        undo_stack->stack.len = undo_stack->len; // truncate redo history
        APPEND(&undo_stack->stack, command);
        undo_stack->len = undo_stack->stack.len;
    }
    
    do_command(cat, &undo_stack->stack.data[undo_stack->len - 1]);
}

void undo_last(Category* cat,UndoStack* undo_stack){
    ASSERT(undo_stack->len);
    undo_command(cat,&undo_stack->stack.data[--undo_stack->len]);
    SHRINK(&undo_stack->stack); //we kinda need to make sure to not overstore these
}

void redo_last(Category* cat, UndoStack* undo_stack) {
    ASSERT(undo_stack->len < undo_stack->stack.len);
    do_command(cat, &undo_stack->stack.data[undo_stack->len++]);
}

void do_command(Category* cat, Command* command) {
    switch (command->type) {
        case CMD_ADD_ELEMENT: {
            Element* elem = allocate_elem(cat);
            // printf("doing add with id %d\n",elem->id.global_id);

            // *elem = (Element){.id = elem->id}; //wrong zeros out the id we got... Zero out others for now
            command->backup = *elem;
            break;
        }
        case CMD_DELETE_ELEMENT: {
            ID id = command->backup.id;
            Element* elem = &cat->elements.data[id.slot];
            // printf("doing delete with id %d\n",elem->id.global_id);

            command->backup = *elem;
            delete_elem(cat, id);
            break;
        }

        case CMD_DUMP_ALL:
            puts("DUMPING:");
            for(size_t i = 0;i<cat->elements.len;i++){
                const Element elem = cat->elements.data[i];
                if(elem.id.global_id>0){
                    print_elem(&elem);
                    putchar('\n');
                }
            }
            break;
    }
}

void undo_command(Category* cat, Command* command) {
    switch (command->type) {
        case CMD_ADD_ELEMENT: {
            // printf("undoing add\n");
            delete_elem(cat, command->backup.id);
            // printf("in undo got%d expected%d\n", (int32_t)cat->global_id,command->backup.id.global_id);
            ASSERT((int32_t)cat->global_id==command->backup.id.global_id);
            
            cat->global_id--;//reduce the global id because adding incremented it
            break;
        }
        case CMD_DELETE_ELEMENT: {
            // printf("undoing del\n");
            revive_elem(cat, command->backup);
            break;
        }

        case CMD_DUMP_ALL:
            break;
    }
}
