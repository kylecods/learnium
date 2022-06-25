#include "ln.h"

#define ALLOCATE_OBJ(vm,type, obj_type) \
    (type*)allocate_object(vm,sizeof(type),obj_type)

static Obj* allocate_object(LnVM* vm, size_t size,ObjType type){
    Obj* object = (Obj*) reallocate(vm,NULL,0,size);

    object->type = type;
    object->is_marked = false;
    object->next = vm->objects;
    vm->objects = object;

#if 0
    printf("%p allocate %zd for %d\n", (void*)object, size,type);
#endif
    return object;
}

ObjModule *new_module(LnVM* vm, ObjString* name){
    Value moduleVal;
    if(table_get(&vm->modules,name,&moduleVal)){
        return AS_MODULE(moduleVal);
    }

    ObjModule* module = ALLOCATE_OBJ(vm,ObjModule,OBJ_MODULE);
    init_table(&module->values);
    module->name = name;
    module->path = NULL;

    //TODO:VM
    ObjString* __file__ = copy_string(vm,"__file__", 8);
    //TODO:pushvm

    table_set(vm,&module->values, __file__, OBJ_VAL(name));
    table_set(vm,&vm->modules, name, OBJ_VAL(module));

    //TODO:VM POPX2
    return module;
}

ObjBoundMethod* new_boundmethod(LnVM* vm, Value receiver, ObjClosure* method){
    ObjBoundMethod* boundMethod = ALLOCATE_OBJ(vm,ObjBoundMethod,OBJ_BOUND_METHOD);
    boundMethod->receiver=receiver;
    boundMethod->method = method;
    return boundMethod;
}

ObjClass* new_class(LnVM* vm, ObjString* name, ObjClass* super_class){
    ObjClass* klass = ALLOCATE_OBJ(vm,ObjClass,OBJ_CLASS);
    klass->name = name;
    klass->super_class = super_class;
    init_table(&klass->methods);
    init_table(&klass->properties);
    return klass;
}

ObjEnum* new_enum(LnVM* vm,ObjString* name){
    ObjEnum* enumObj = ALLOCATE_OBJ(vm,ObjEnum,OBJ_ENUM);
    enumObj->name = name;
    init_table(&enumObj->values);
    return enumObj;
}

ObjClosure* new_closure(LnVM* vm,ObjFun* function){
    ObjUpvalue **upvalues = ALLOCATE(vm,ObjUpvalue*, function->upvalue_count);
    for (int i = 0; i < function->upvalue_count; i++) {
        upvalues[i] = NULL;
    }
    ObjClosure* closure = ALLOCATE_OBJ(vm,ObjClosure,OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalue_count = function->upvalue_count;
    return closure;
}

ObjFun* new_function(LnVM* vm, ObjModule* module){
    ObjFun* function = ALLOCATE_OBJ(vm,ObjFun,OBJ_FUNCTION);
    function->arity = 0;
    function->upvalue_count = 0;
    function->name = NULL;
    function->module = module;
    init_chunk(&function->chunk);
    return function;
}

ObjInstance* new_instance(LnVM* vm, ObjClass* klass){
    ObjInstance* instance = ALLOCATE_OBJ(vm,ObjInstance,OBJ_INSTANCE);
    instance->klass = klass;
    init_table(&instance->fields);

    //TODO:VMpsuh
    ObjString* classString = copy_string(vm,"_class", 6);
    //TODO:VMpush
    table_set(vm,&instance->fields,classString,OBJ_VAL(klass));

    //TODO:Popx2
    return instance;
}

ObjNative* new_native(LnVM* vm, NativeFn function){
    ObjNative* native = ALLOCATE_OBJ(vm,ObjNative,OBJ_NATIVE);
    native->function = function;
    return native;
}
static ObjString* allocate_string(LnVM* vm, char* chars,int length, uint32_t hash){
    ObjString* string = ALLOCATE_OBJ(vm,ObjString,OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    //TODO:VMPUSHGC
    table_set(vm,&vm->strings,string,NIL_VAL);
    //TODO:VMPOPGC
    return string;
}
static uint32_t hash_string(const char* key, int length){
    uint32_t hash = 2166136261u;

    for (int i = 0; i < length; i++) {
        hash ^= key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString* take_string(LnVM* vm,char* chars, int length){
    uint32_t hash = hash_string(chars,length);
    ObjString* interned_string = table_find_string(&vm->strings, chars,length,hash);
    if(interned_string != NULL){
        FREE_ARRAY(vm,char,chars,length+1);
        return interned_string;
    }
    //terminate char is present
    chars[length] = '\0';
    return allocate_string(vm,chars,length,hash);
}

ObjString* copy_string(LnVM* vm,const char* chars, int length){
    uint32_t hash = hash_string(chars,length);
    ObjString* interned_string = table_find_string(&vm->strings, chars,length,hash);
    if(interned_string != NULL) return interned_string;

    char* heap_chars = ALLOCATE(vm,char,length + 1);
    memcpy(heap_chars,chars,length);
    heap_chars[length] = '\0';
    return allocate_string(vm,heap_chars,length,hash);
}

ObjList* new_list(LnVM* vm){
    ObjList* list = ALLOCATE_OBJ(vm,ObjList,OBJ_LIST);
    init_valueArray(&list->values);
    return list;
}

ObjMap* new_map(LnVM* vm){
    ObjMap* map = ALLOCATE_OBJ(vm,ObjMap, OBJ_MAP);
    map->capacity_mask = - 1;
    map->count = 0;
    map->entries = NULL;
    return map;
}

ObjFile* new_file(LnVM* vm){
    return ALLOCATE_OBJ(vm,ObjFile,OBJ_FILE);
}

ObjUpvalue* new_upvalue(LnVM* vm, Value* slot){
    ObjUpvalue* upvalue = ALLOCATE_OBJ(vm,ObjUpvalue,OBJ_UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->value = slot;
    upvalue->next = NULL;
    return upvalue;
}