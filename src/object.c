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

ObjArray* new_array(LnVM* vm){
    ObjArray* array = ALLOCATE_OBJ(vm,ObjArray,OBJ_ARRAY);
    init_valueArray(&array->values);
    return array;
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

static void print_array(ObjArray* array)
{
	printf("[");
    for (int i = 0; i < array->values.count; i++)
    {
        print_value(array->values.value[i]);

        if (i != array->values.count - 1) 
        {
            printf(", ");
        }
    }
	printf("]");

}

static void print_class(ObjClass* klass)
{
    printf("<Class %s>", klass->name->chars);
}

static void print_function(ObjFun* function)
{
    if (function->name == NULL) 
    {
        printf("<script>");
	}
	else if (function->name->length == 0 || function->name->chars[0] == '{')
    {
        printf("<fn>");
    } 
    else
    {
        printf("<fn %s>", function->name->chars);
    }
}

static void print_map(ObjMap* map){
	printf("{");
	int start_index = 0;
	for (int i = 0; i < map->capacity_mask; i++) {
        MapEntry* entry = &map->entries[i];

        if (IS_EMPTY(entry->key)) {
			continue;
        }
        print_value(entry->key);
        printf(": ");
        print_value(entry->value);
		start_index = i + 1;
        break;
    }
    
    for (int i = start_index; i <= map->capacity_mask; i++) {
        MapEntry* entry = &map->entries[i];
        if (IS_EMPTY(entry->key)) {
			continue;
        }

		printf(", ");
        print_value(entry->key);
        printf(": ");
		print_value(entry->value);
    }

	printf("}");
}

void print_object(Value value){
    switch (OBJ_TYPE(value)) 
    {
        case OBJ_MODULE:{
            ObjModule* module = AS_MODULE(value);
            char* module_string = malloc(sizeof(char) * (module->name->length + 11));
            snprintf(module_string,(module->name->length + 10), "<Module %s>",module->name->chars);
            return module_string;
        }
        case OBJ_CLASS:{
			print_class(AS_CLASS(value));
            break;
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
			print_function(AS_CLOSURE(value)->function);
            break;
        }
        case OBJ_FUNCTION:{
			print_function(AS_FUNC(value));
            break;
        }
        //case OBJ_INSTANCE:{
        //    ObjInstance* instance = AS_INSTANCE(value);
        //    char* instance_string = malloc(sizeof(char) * (instance->klass->name->length + 12));
        //    memcpy(instance_string, "<", 1);
        //    memcpy(instance_string + 1, instance->klass->name->chars,instance->klass->name->length);
        //    memcpy(instance_string + 1 + instance->klass->name->length, " instance>",10);
        //    instance_string[instance->klass->name->length + 11] = '\0';
        //    return instance_string;
        //}
        case OBJ_STRING:{
			printf("%s", AS_CSTRING(value));
            break;
        }
        case OBJ_NATIVE:{
			printf("<native fn>");
            break;
        }
        case OBJ_FILE:{
            ObjFile* file = AS_FILE(value);
            char* file_string = malloc(sizeof(char) * (strlen(file->path) + 8));
            snprintf(file_string, strlen(file->path) + 8, "<File %s>", file->path);
            return file_string;
        }
        case OBJ_MAP:{
             print_map(AS_MAP(value));
             break;
        }
        case OBJ_ARRAY:{
            print_array(AS_ARRAY(value));
            break;
        }
        case OBJ_UPVALUE:{
			printf("upvalue");
            break;
        }
         default:
            printf("unknown");
			break;
    }
}