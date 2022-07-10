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

    push(vm, OBJ_VAL(module));
    ObjString* __file__ = copy_string(vm,"__file__", 8);
    push(vm, OBJ_VAL(__file__));

    table_set(vm,&module->values, __file__, OBJ_VAL(name));
    table_set(vm,&vm->modules, name, OBJ_VAL(module));

    pop(vm);
    pop(vm);
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

    push(vm, OBJ_VAL(instance));
    ObjString* classString = copy_string(vm,"_class", 6);
    push(vm, OBJ_VAL(classString));
    table_set(vm,&instance->fields,classString,OBJ_VAL(klass));

    pop(vm);
    pop(vm);
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
    push(vm, OBJ_VAL(string));
    table_set(vm,&vm->strings,string,NIL_VAL);
    pop(vm);
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
char* list_to_string(Value value){
    int size = 50;
    ObjList* list = AS_LIST(value);
    char* list_string = malloc(sizeof(char) * size);
    memcpy(list_string, "[", 1);

    int list_string_length = 1;
    for (int i = 0; i < list->values.count; i++) {
        Value  list_value = list->values.value[i];
        char* element;
        int element_size;
        if(IS_STRING(list_value)){
            ObjString* s = AS_STRING(list_value);
            element = s->chars;
            element_size = s->length;
        }else{
            element = value_to_string(list_value);
            element_size = strlen(element);
        }

        if(element_size > (size - list_string_length - 6)){
            if(element_size > size){
                size = size + element_size * 2 + 6;
            } else{
                size = size * 2 + 6;
            }
            char* newB = realloc(list_string,sizeof(char) *size);
            if(newB == NULL){
                printf("Unable to allocate memory\n");
                exit(71);
            }
            list_string = newB;
        }
        if(IS_STRING(list_value)){
            memcpy(list_string+list_string_length, "\"", 1);
            memcpy(list_string + list_string_length + 1, element,element_size);
            memcpy(list_string + list_string_length + 1 + element_size,"\"", 1);
            list_string_length += element_size + 2;
        } else{
            memcpy(list_string + list_string_length,element,element_size);
            list_string_length += element_size;
            free(element);
        }
        if(i != list->values.count - 1){
            memcpy(list_string + list_string_length, ", ", 2);
            list_string_length+=2;
        }
    }
    memcpy(list_string + list_string_length, "]", 1);
    list_string[list_string_length + 1] = '\0';
    return list_string;
}

char* map_to_string(Value value){
    int count = 0;
    int size = 50;
    ObjMap* map = AS_MAP(value);
    char* map_string = malloc(sizeof(char) * size);
    memcpy(map_string, "{", 1);
    int map_string_length = 1;

    for (int i = 0; i <= map->capacity_mask; i++) {
        MapEntry* item = &map->entries[i];
        if(IS_EMPTY(item->key)) continue;

        count++;

        char* key;
        int key_size;
        if(IS_STRING(item->key)){
            ObjString* s = AS_STRING(item->key);
            key = s->chars;
            key_size = s->length;
        }else{
            key = value_to_string(item->key);
            key_size = strlen(key);
        }

        if(key_size > (size - map_string_length - key_size - 4)){
            if(key_size > size){
                size += key_size * 2 + 4;
            } else{
                size *= 2 + 4;
            }
            char* newB = realloc(map_string, sizeof(char) * size);

            if(newB == NULL){
                printf("Unable to allocate memory\n");
                exit(71);
            }
            map_string = newB;
        }

        if(IS_STRING(item->key)){
            memcpy(map_string+map_string_length, "\"", 1);
            memcpy(map_string + map_string_length + 1, key,key_size);
            memcpy(map_string + map_string_length + 1 + key_size,"\": ", 3);
            map_string_length += key_size + 2;
        } else{
            memcpy(map_string + map_string_length,key,key_size);
            memcpy(map_string + map_string_length + key_size, ": ",2);
            map_string_length += key_size + 2;
            free(key);
        }

        char* element;
        int element_size;

        if(IS_STRING(item->value)){
            ObjString* s = AS_STRING(item->value);
            element = s->chars;
            element_size = s->length;
        } else{
            element = value_to_string(item->value);
            element_size = strlen(element);
        }

        if(element_size > (size - map_string_length - element_size - 6)){
            if(element_size > size){
                size += element_size * 2 + 6;
            } else{
                size = size * 2 + 6;
            }

            char* newB = realloc(map_string,sizeof (char) * size);

            if(newB == NULL){
                printf("Unable to allocate memory\n");
                exit(71);
            }
            map_string = newB;
        }

        if(IS_STRING(item->value)){
            memcpy(map_string+map_string_length, "\"", 1);
            memcpy(map_string + map_string_length + 1, element,element_size);
            memcpy(map_string + map_string_length + 1 + element_size,"\"", 1);
            map_string_length += element_size + 2;
        } else{
            memcpy(map_string + map_string_length,element,element_size);
            map_string_length += element_size;
            free(element);
        }
        if(count != map->count){
            memcpy(map_string + map_string_length, ", ", 2);
            map_string_length +=2;
        }
    }
    memcpy(map_string + map_string_length, "}", 1);
    map_string[map_string_length + 1] = '\0';

    return map_string;
}

char* object_to_string(Value value){
    switch (AS_OBJ(value)->type) {
        case OBJ_MODULE:{
            ObjModule* module = AS_MODULE(value);
            char* module_string = malloc(sizeof(char) * (module->name->length + 11));
            snprintf(module_string,(module->name->length + 10), "<Module %s>",module->name->chars);
            return module_string;
        }
        case OBJ_CLASS:{
            ObjClass* klass = AS_CLASS(value);
            char* class_string = malloc(sizeof(char) * (klass->name->length + 7));
            memcpy(class_string, "<Class ",5);
            memcpy(class_string + 5, klass->name->chars,klass->name->length);
            memcpy(class_string + 5 + klass->name->length, ">", 1);
            class_string[klass->name->length + 6] = '\0';
            return class_string;
        }
        case OBJ_ENUM:{
            ObjEnum* enumObj = AS_ENUM(value);
            char* enum_string = malloc(sizeof(char) * (enumObj->name->length + 8));
            memcpy(enum_string, "<Enum ",6);
            memcpy(enum_string + 6, enumObj->name->chars,enumObj->name->length);
            memcpy(enum_string + 6 + enumObj->name->length, ">", 1);
            enum_string[enumObj->name->length + 7] = '\0';
            return enum_string;
        }
        case OBJ_BOUND_METHOD:{
            ObjBoundMethod* method = AS_BOUND_METHOD(value);
            char* method_string;
            if(method->method->function->name != NULL){
                method_string = malloc(sizeof(char) * (method->method->function->name->length + 16));
                snprintf(method_string,method->method->function->name->length + 16, "<Bound Method %s>",method->method->function->name->chars);
            } else{
                method_string = malloc(sizeof(char) * 15);
                memcpy(method_string, "<Bound Method>", 14);
                method_string[14] = '\0';
            }

            return method_string;
        }
        case OBJ_CLOSURE:{
            ObjClosure* closure = AS_CLOSURE(value);
            char* closure_string;

            if(closure->function->name != NULL){
                closure_string = malloc(sizeof(char) * (closure->function->name->length + 6));
                snprintf(closure_string,closure->function->name->length + 6, "<fn %s>",closure->function->name->chars);
            } else{
                closure_string= malloc(sizeof(char) * 9);
                memcpy(closure_string, "<Script>", 8);
                closure_string[8] = '\0';
            }
            return closure_string;
        }
        case OBJ_FUNCTION:{
            ObjFun* function = AS_FUNC(value);
            char* function_string;
            if(function->name !=NULL){
                function_string = malloc(sizeof(char)* (function->name->length + 6));
                snprintf(function_string, function->name->length + 6, "<fn %s>", function->name->chars);
            } else{
                function_string = malloc(sizeof(char) * 5);
                memcpy(function_string, "<fn>", 4);
                function_string[4] = '\0';
            }
            return function_string;
        }
        case OBJ_INSTANCE:{
            ObjInstance* instance = AS_INSTANCE(value);
            char* instance_string = malloc(sizeof(char) * (instance->klass->name->length + 12));
            memcpy(instance_string, "<", 1);
            memcpy(instance_string + 1, instance->klass->name->chars,instance->klass->name->length);
            memcpy(instance_string + 1 + instance->klass->name->length, " instance>",10);
            instance_string[instance->klass->name->length + 11] = '\0';
            return instance_string;
        }
        case OBJ_STRING:{
            ObjString* stringObj = AS_STRING(value);
            char* string = malloc(sizeof(char)*stringObj->length +1);
            memcpy(string,stringObj->chars, stringObj->length);
            string[stringObj->length] = '\0';
            return string;
        }
        case OBJ_NATIVE:{
            char* native_string = malloc(sizeof(char) * 12);
            memcpy(native_string,"<fn native>", 11);
            native_string[11] = '\0';
            return native_string;
        }
        case OBJ_FILE:{
            ObjFile* file = AS_FILE(value);
            char* file_string = malloc(sizeof(char) * (strlen(file->path) + 8));
            snprintf(file_string, strlen(file->path) + 8, "<File %s>", file->path);
            return file_string;
        }
        case OBJ_MAP:{
            return map_to_string(value);
        }
        case OBJ_LIST:{
            return list_to_string(value);
        }
        case OBJ_UPVALUE:{
            char* upvalue_string = malloc(sizeof(char) * 8);
            memcpy(upvalue_string,"upvalue", 7);
            upvalue_string[7] = '\0';
            return upvalue_string;
        }
    }
    char *unknown = malloc(sizeof(char) * 9);
    snprintf(unknown, 8, "%s","unknown");
    return unknown;
}