#include <stdlib.h>
#include <string.h>
#include "commands.h"

void do_command(Category* cat, Command* command) {
    switch (command->type) {
        case CMD_ADD_ELEMENT: {
            Element* elem = allocate_elem(cat);
            *elem = (Element){.id = elem->id}; // Zero out others for now
            command->backup = *elem;
            break;
        }
        case CMD_DELETE_ELEMENT: {
            ID id = command->backup.id;
            Element* elem = &cat->elements.data[id.slot];
            command->backup = *elem;
            delete_elem(cat, id);
            break;
        }
    }
}

void undo_command(Category* cat, Command* command) {
    switch (command->type) {
        case CMD_ADD_ELEMENT: {
            delete_elem(cat, command->backup.id);
            break;
        }
        case CMD_DELETE_ELEMENT: {
            revive_elem(cat, command->backup);
            break;
        }
    }
}
