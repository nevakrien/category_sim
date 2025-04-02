/**
 * this module is representing an infinity Category
 * things are allocated in memory as tight as we can make them
 */

#ifndef CATEGORY_H
#define CATEGORY_H

#include <stdint.h>
#include "utils.h"

//should be 1 register
typedef struct __attribute__((aligned(8))) ID {
	int32_t slot;
	int32_t global_id;
} ID;


// DEFINE_VECTOR_TYPE(void*,AnyVector);

typedef struct Element Element;
DEFINE_VECTOR_TYPE(Element,ElemArray);
DEFINE_VECTOR_TYPE(uint32_t,U32Array);


typedef struct Type{
	char* name;//owned
	// ElemArray memebers;
}Type;


struct Element{
	const Type* type;
	void* data;
	ID id;
};

typedef struct Arrow{
	ID source;
	ID target;
}Arrow;

static inline void print_elem(const Element* elem){
	// printf("[%d]",elem->id.global_id);
	ASSERT(elem->type);
	printf("%s [%d]",elem->type->name,elem->id.global_id);
}

typedef struct {
	uint32_t global_id;

	ElemArray elements;
	U32Array free_list;

}Category;

static inline Element* get_elem(Category* cat,ID id){
	ASSERT((size_t)id.slot<cat->elements.len);
	Element* ans = &cat->elements.data[id.slot];
	ASSERT(ans->id.slot==id.slot);
	if(ans->id.global_id!=id.global_id)
		return NULL;
	return ans;
}


//this function allways returns a non null ID to a new element
//note that this pointer is invalidated by the next allocation
static inline Element* allocate_elem(Category* cat) {
	ID id;
	
	if (cat->free_list.len > 0) {
		id.slot = cat->free_list.data[--cat->free_list.len];
		// SHRINK(cat->free_list); //we might wana save on memory
	} else {
		id.slot = cat->elements.len++;
		ENSURE_CAPACITY(&cat->elements);
	}

	id.global_id = ++cat->global_id;
	Element* ans = &cat->elements.data[id.slot];

	//make sure we are writing to an empty slot
	ASSERT(ans->id.slot<=0 && ans->id.global_id<=0);
	ans->id = id;

	return ans;
}

//this should only be used when it is expected that the slot is free.
//it is here to allow undo to work
static inline Element* revive_elem(Category* cat,Element ref){
	Element* elem = &cat->elements.data[ref.id.slot];
	ASSERT(elem->id.slot ==-1);
	*elem = ref;
	return elem;
}

static inline void delete_elem(Category* cat,ID id) {
	ASSERT(id.global_id >0 && 0<=cat->elements.data[id.slot].id.slot && 0<cat->elements.data[id.slot].id.global_id);
	APPEND(&cat->free_list,id.slot);
	cat->elements.data[id.slot].id = (ID){.slot = -1,.global_id = -1};
}

static inline void free_category(Category* cat) {
    free(cat->elements.data);
    free(cat->free_list.data);
    *cat = (Category){0}; // optional: zero out everything
}


#endif //CATEGORY_H
