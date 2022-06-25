#include "ln.h"

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(LnVM* vm, void* previous, size_t old_size,size_t new_size){
    vm->bytes_allocated += new_size-old_size;

#if 0
    printf("Total bytes allocated: %zu\nNew allocation: %zu\nOld allocation: %zu\n\n", vm->bytes_allocated, new_size,old_size);
#endif
    if(new_size > old_size){
#if 0
        collect_garbage(vm);
#endif
        if(vm->bytes_allocated > vm->next_gc){
            collect_garbage(vm);
        }
    }
    if(new_size == 0){
        free(previous);
        return NULL;
    }
    return realloc(previous,new_size);
}

void gray_object(LnVM* vm, Obj* object){
    if(object == NULL) return;

    if(object->is_marked) return;

#if 0
    printf("%p gray ", (void *)object);
    print_value(OBJ_VAL(object));
    printf("\n");
#endif

    object->is_marked = true;
    if(vm->gray_capacity < vm->gray_count + 1){
        vm->gray_capacity = GROW_CAPACITY(vm->gray_capacity);

        vm->gray_stack = realloc(vm->gray_stack, sizeof(Obj *) * vm->gray_capacity);
    }
    vm->gray_stack[vm->gray_count++] = object;
}
void gray_value(LnVM* vm, Value value){
    if(IS_OBJ(value)) return;

    gray_object(vm, AS_OBJ(value));
}

void gray_array(LnVM* vm, ValueArray* array){
    for(int i = 0; i< array->count; i++){
        gray_value(vm,array->value[i]);
    }
}

static void blacken_object(LnVM* vm, Obj* object){
#if 0
    printf("%p blacken ", (void *)object);
    print_value(OBJ_VAL(object));
    printf("\n");
#endif
    switch (object->type) {
        case OBJ_MODULE: {
            ObjModule *module = (ObjModule *) object;
            gray_object(vm, (Obj *) module->name);
            gray_object(vm, (Obj *) module->path);
            gray_table(vm,&module->values);
            break;
        }
        case OBJ_BOUND_METHOD:{
            ObjBoundMethod* bound = (ObjBoundMethod*)object;
            gray_value(vm,bound->receiver);
            gray_object(vm,(Obj*) bound->method);
            break;
        }
        case OBJ_CLASS:{
            ObjClass* klass = (ObjClass*) object;
            gray_object(vm,(Obj*)klass->name);
            gray_object(vm,(Obj*)klass->super_class);
            gray_table(vm,&klass->methods);
            gray_table(vm,&klass->properties);
            break;
        }
        case OBJ_ENUM:{
            ObjEnum* enumObj = (ObjEnum*) object;
            gray_object(vm,(Obj*)enumObj->name);
            gray_table(vm,&enumObj->values);
            break;
        }
        case OBJ_CLOSURE:{
            ObjClosure* closure = (ObjClosure*)object;
            gray_object(vm,(Obj*)closure->function);\
            for (int i = 0; i < closure->upvalue_count; ++i) {
                gray_object(vm,(Obj *)closure->upvalues[i]);
            }
            break;
        }
        case OBJ_FUNCTION:{
            ObjFun* function = (ObjFun*) object;
            gray_object(vm,(Obj*)function->name);
            gray_array(vm,&function->chunk.constants);
            break;
        }
        case OBJ_INSTANCE:{
            ObjInstance* instance = (ObjInstance*)object;
            gray_object(vm,(Obj*)instance->klass);
            gray_table(vm,&instance->fields);
            break;
        }
        case OBJ_UPVALUE:{
            gray_value(vm,((ObjUpvalue*)object)->closed);
            break;
        }
        case OBJ_LIST:{
            ObjList* list = (ObjList*)object;
            gray_array(vm,&list->values);
            break;
        }
        case OBJ_MAP:{
            ObjMap* map = (ObjMap*)object;
            gray_map(vm, map);
            break;
        }
        case OBJ_FILE:
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
    }
}

void free_object(LnVM* vm, Obj* object){
#if 0
    printf("%p free type %d\n", (void*)object, object->type);
#endif

    switch (object->type) {
        case OBJ_MODULE:{
            ObjModule* module = (ObjModule*)object;
            free_table(vm,&module->values);
            FREE(vm,ObjModule, object);
            break;
        }
        case OBJ_BOUND_METHOD:{
            FREE(vm,ObjBoundMethod,object);
            break;
        }
        case OBJ_CLASS:{
            ObjClass* klass = (ObjClass*)object;
            free_table(vm,&klass->methods);
            free_table(vm,&klass->properties);
            FREE(vm,ObjClass,object);
        }
        case OBJ_ENUM:
        {
            ObjEnum* enumObj = (ObjEnum*)object;
            free_table(vm,&enumObj->values);
            FREE(vm,ObjEnum,object);
            break;
        }
        case OBJ_CLOSURE:{
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(vm,ObjUpvalue*,closure->upvalues, closure->upvalue_count);
            FREE(vm,ObjClosure,object);
            break;
        }
        case OBJ_FUNCTION:{
            ObjFun* function = (ObjFun*)object;
            free_chunk(vm,&function->chunk);
            FREE(vm,ObjFun,object);
            break;
        }
        case OBJ_INSTANCE:{
            ObjInstance* instance = (ObjInstance*)object;
            free_table(vm,&instance->fields);
            FREE(vm,ObjInstance,object);
            break;
        }
        case OBJ_NATIVE:{
            FREE(vm,ObjNative,object);
            break;
        }
        case OBJ_STRING:{
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(vm,char,string->chars,string->length +1);
            FREE(vm,ObjString,string);
            break;
        }
        case OBJ_LIST:{
            ObjList* list = (ObjList*)object;
            free_valueArray(vm,&list->values);
            FREE(vm,ObjList,list);
            break;
        }
        case OBJ_MAP:{
            ObjMap* map = (ObjMap*)object;
            FREE_ARRAY(vm,MapEntry,map->entries,map->capacity_mask + 1);
            FREE(vm,ObjMap,map);
            break;
        }
        case OBJ_FILE:{
            FREE(vm,ObjFile,object);
            break;
        }
        case OBJ_UPVALUE:{
            FREE(vm,ObjUpvalue,object);
            break;
        }

    }
}

void collect_garbage(LnVM* vm){
#if 0
    printf("-- GC begin --\n");
    size_t before = vm->bytes_allocated;
#endif

    //mark process
    for (Value *slot = vm->stack; slot < vm->stack_top; slot++) {
        gray_value(vm,*slot);
    }
    for (int i = 0; i < vm->frame_count; ++i) {
        gray_object(vm,(Obj*) vm->frames[i].closure);
    }
    for (ObjUpvalue* upvalue = vm->open_upvalues; upvalue != NULL; upvalue= upvalue->next) {
        gray_object(vm,(Obj*)upvalue);
    }

    gray_table(vm,&vm->modules);
    gray_table(vm,&vm->globals);
    gray_table(vm,&vm->list_methods);
    gray_table(vm,&vm->string_methods);
    gray_table(vm,&vm->map_methods);
    gray_table(vm,&vm->file_methods);

    //TODO:gray COMPILER ROOTS

    gray_object(vm,(Obj*) vm->init_string);


    //trace references
    while (vm->gray_count > 0){
        Obj* object = vm->gray_stack[--vm->gray_count];
        blacken_object(vm,object);
    }

    //delete unused interned strings
    table_remove_whites(vm,&vm->strings);


    //sweep or collect the objects
    Obj **object = &vm->objects;
    while (*object !=NULL){
        if(!((*object)->is_marked)){
            Obj* unreached = *object;
            *object = unreached->next;
            free_object(vm,unreached);
        } else{
            (*object)->is_marked= false;
            object = &(*object)->next;
        }
    }

    vm->next_gc = vm->bytes_allocated * GC_HEAP_GROW_FACTOR;

#if 0
    printf("--gc collected %ld bytes (from %ld to %ld) next at %ld\n", before - vm->bytes_allocated,before,vm->bytes_allocated,vm->next_gc);
#endif
}
void free_objects(LnVM* vm){
    Obj* object = vm->objects;
    while (object != NULL){
        Obj* next = object->next;
        free_object(vm,object);
        object = next;
    }
    free(vm->gray_stack);
}