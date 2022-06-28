#include "ln.h"

static void reset_stack(LnVM* vm){
    vm->stack_top = vm->stack;
    vm->frame_count = 0;
    vm->compiler = NULL;
}

void runtime_error(LnVM* vm, const char* format, ...){
    for (int i = vm->frame_count - 1; i > 0; i--) {
        CallFrame* frame = &vm->frames[i];

        ObjFun * function = frame->closure->function;

        size_t instruction = frame->ip - function->chunk.code - 1;

        if(function->name == NULL){
            fprintf(stderr, "File '%s', [line %d]\n", function->module->name->chars, function->chunk.lines[instruction]);
            i = -1;
        } else{
            fprintf(stderr, "Function '%s' in '%s', [line %d]\n", function->name->chars,function->module->name->chars, function->chunk.lines[instruction]);
        }
        va_list args;
                va_start(args, format);
        vfprintf(stderr,format,args);
        fputs("\n",stderr);
        va_end(args);
    }

    reset_stack(vm);

}

LnVM* init_vm(int argc, char **argv){
    LnVM* vm = malloc(sizeof(*vm));

    if(vm == NULL){
        printf("Failed to allocate memory");
        exit(71);
    }

    memset(vm,'\0', sizeof(LnVM));

    reset_stack(vm);
    vm->objects = NULL;
    vm->frame_capacity = 4;
    vm->frames = NULL;
    vm->init_string = NULL;
    vm->bytes_allocated = 0;
    vm->next_gc = 1024 * 1024;
    vm->gray_count =0;
    vm->gray_capacity = 0;
    vm->gray_stack = NULL;
    vm->last_module = NULL;
    vm->argc = argc;
    vm->argv = argv;
    init_table(&vm->modules);
    init_table(&vm->globals);
    init_table(&vm->strings);


    init_table(&vm->string_methods);
    init_table(&vm->file_methods);
    init_table(&vm->list_methods);
    init_table(&vm->map_methods);

    vm->frames = ALLOCATE(vm,CallFrame,vm->frame_count);
    vm->init_string = copy_string(vm,"init",4);

    //TODO:Native functions

    //TODO: Native methods

    return vm;

}

void free_vm(LnVM* vm){
    free_table(vm,&vm->modules);
    free_table(vm,&vm->globals);
    free_table(vm,&vm->strings);

    free_table(vm,&vm->string_methods);
    free_table(vm,&vm->file_methods);
    free_table(vm,&vm->list_methods);
    free_table(vm,&vm->map_methods);

    FREE_ARRAY(vm,CallFrame,vm->frames, vm->frame_capacity);
    vm->init_string = NULL;
    free_objects(vm);
    free(vm);
}

void push(LnVM* vm, Value value){
    *vm->stack_top = value;
    vm->stack_top++;
}
Value pop(LnVM* vm){
    vm->stack_top--;
    return *vm->stack_top;
}

Value peek(LnVM* vm, int distance){
    return vm->stack_top[-1 - distance];
}

ObjClosure* compile_module_to_closure(LnVM* vm,char* name, char* source){
    ObjString* pathObj = copy_string(vm,name, strlen(name));
    push(vm,OBJ_VAL(pathObj));
    ObjModule* module = new_module(vm,pathObj);
    pop(vm);
    push(vm, OBJ_VAL(module));
    ObjFun* function = compile(vm,module,source);
    pop(vm);
    if(function == NULL) return NULL;
    push(vm, OBJ_VAL(function));
    ObjClosure* closure = new_closure(vm,function);
    pop(vm);
    return closure;
}
static bool call(LnVM* vm, ObjClosure* closure,int arg_count){
    if(arg_count < closure->function->arity){
        runtime_error(vm,"Function '%s' expected %d argument(s) but got %d.", closure->function->name->chars,closure->function->arity,arg_count);
        return false;
    }
    if(vm->frame_count == vm->frame_capacity){
        int old_capacity = vm->frame_capacity;
        vm->frame_capacity = GROW_CAPACITY(vm->frame_capacity);
        vm->frames = GROW_ARRAY(vm,vm->frames, CallFrame,old_capacity,vm->frame_capacity);
    }

    CallFrame* frame = &vm->frames[vm->frame_count++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;

    frame->slots = vm->stack_top - arg_count -1;
    return true;
}
static bool call_value(LnVM* vm, Value callee, int arg_count){
    if(IS_OBJ(callee)){
        switch (AS_OBJ(callee)->type) {
            case OBJ_BOUND_METHOD:{
                ObjBoundMethod* boundMethod = AS_BOUND_METHOD(callee);
                vm->stack_top[-arg_count - 1] = boundMethod->receiver;
                return call(vm,boundMethod->method,arg_count);
            }
            case OBJ_CLASS:{
                ObjClass* klass = AS_CLASS(callee);\
                vm->stack_top[-arg_count - 1] = OBJ_VAL(new_instance(vm,klass));
                Value initializer;
                if(table_get(&klass->methods, vm->init_string, &initializer)){
                    return call(vm, AS_CLOSURE(initializer),arg_count);
                }else if(arg_count != 0){
                    runtime_error(vm,"Expected 0 arguments but got %d.",arg_count);
                    return false;
                }

                return true;
            }
            case OBJ_CLOSURE:{
                vm->stack_top[-arg_count - 1] = callee;
                return call(vm, AS_CLOSURE(callee),arg_count);
            }
            case OBJ_NATIVE:{
                NativeFn native = AS_NATIVE(callee);
                Value result = native(vm,arg_count,vm->stack_top - arg_count);
                if(IS_EMPTY(result)) return false;

                vm->stack_top -= arg_count + 1;
                push(vm,result);
                return true;
            }
            default:
                break;
        }
    }

    runtime_error(vm,"Can only call classes or functions");
    return false;
}

static bool call_native_method(LnVM* vm, Value method,int arg_count){
    NativeFn native = AS_NATIVE(method);
    Value result = native(vm,arg_count,vm->stack_top - arg_count);
    if(IS_EMPTY(result)) return false;

    vm->stack_top -= arg_count + 1;
    push(vm,result);
    return true;
}

static bool invoke_from_class(LnVM* vm,ObjClass* klass,ObjString* name,int arg_count){
    Value method;
    if (!table_get(&klass->methods, name, &method)){
        runtime_error(vm, "Undefined property '%s'.", name->chars);
        return false;
    }
    return call(vm, AS_CLOSURE(method),arg_count);
}

static bool invoke(LnVM* vm,ObjString* name, int arg_count){
    Value receiver = peek(vm,arg_count);

    switch (AS_OBJ(receiver)->type) {
        case OBJ_MODULE:{
            ObjModule* module = AS_MODULE(receiver);
            Value value;
            if(!table_get(&module->values, name, &value)){
                runtime_error(vm,"Undefined property '%s'.", name->chars);
                return false;
            }
            return call_value(vm,value,arg_count);
        }
        case OBJ_CLASS:{
            ObjClass* instance = AS_CLASS(receiver);
            Value method;
            if(table_get(&instance->methods, name,&method)){
                return call_value(vm,method,arg_count);
            }
            runtime_error(vm,"Undefined property '%s'.",name->chars);
            return false;
        }
        case OBJ_INSTANCE:{
            ObjInstance* instance = AS_INSTANCE(receiver);
            Value value;
            if(table_get(&instance->fields,name,&value)){
                vm->stack_top[-arg_count - 1] = value;
                return call_value(vm,value,arg_count);
            }
            runtime_error(vm,"Undefined property '%s'",name->chars);
            return false;
        }
        case OBJ_STRING:{
            Value value;
            if(table_get(&vm->string_methods,name,&value)){
                return call_native_method(vm,value,arg_count);
            }
            runtime_error(vm,"String has no method %s().", name->chars);
            return false;
        }
        case OBJ_LIST:{
            Value value;
            if(table_get(&vm->list_methods,name,&value)){
                if(IS_NATIVE(value)) return call_native_method(vm,value,arg_count);

                push(vm, peek(vm,0));

                for (int i = 2; i <= arg_count + 1; i++){
                    vm->stack_top[-i] = peek(vm, i);
                }
                return call(vm, AS_CLOSURE(value),arg_count + 1);
            }
            runtime_error(vm,"List has no method %s()",name->chars);
            return false;
        }
        case OBJ_MAP:{
            Value value;
            if(table_get(&vm->map_methods,name,&value)){
                if(IS_NATIVE(value)) return call_native_method(vm,value,arg_count);

                push(vm, peek(vm,0));

                for (int i = 2; i <= arg_count + 1; i++){
                    vm->stack_top[-i] = peek(vm, i);
                }
                return call(vm, AS_CLOSURE(value),arg_count + 1);
            }
            runtime_error(vm,"Map has no method %s()",name->chars);
            return false;
        }
        case OBJ_FILE:{
            Value value;
            if(table_get(&vm->file_methods,name,&value)){
                return call_native_method(vm,value,arg_count);
            }
            runtime_error(vm,"File has no method %s().", name->chars);
            return false;
        }
        case OBJ_ENUM:{
            ObjEnum* enumObj = AS_ENUM(receiver);

            Value value;
            if(table_get(&enumObj->values,name,&value)){
                return call_value(vm,value,arg_count);
            }

            runtime_error(vm,"'%s' enum has no property '%s'.",enumObj->name->chars,name->chars);
            return false;
        }
        default:
            break;
    }
    runtime_error(vm,"Only instances have methods.");
    return false;
}
static bool bind_method(LnVM* vm, ObjClass* klass,ObjString* name){
    Value method;
    if(!table_get(&klass->methods,name, &method)){
        return false;
    }
    ObjBoundMethod* boundMethod = new_boundmethod(vm,peek(vm,0),AS_CLOSURE(method));
    pop(vm);
    push(vm, OBJ_VAL(boundMethod));
    return true;
}

static ObjUpvalue* capture_upvalue(LnVM* vm, Value* local){
    if(vm->open_upvalues == NULL){
        vm->open_upvalues = new_upvalue(vm,local);
        return vm->open_upvalues;
    }

    ObjUpvalue* pre_upvalue = NULL;
    ObjUpvalue* upvalue = vm->open_upvalues;

    while (upvalue != NULL && upvalue->value > local){
        pre_upvalue=upvalue;
        upvalue=upvalue->next;
    }

    if(upvalue != NULL && upvalue->value == local) return upvalue;

    ObjUpvalue* created_upvalue = new_upvalue(vm,local);
    created_upvalue->next = upvalue;

    if (pre_upvalue == NULL){
        vm->open_upvalues = created_upvalue;
    } else{
        pre_upvalue->next = created_upvalue;
    }

    return created_upvalue;
}
static void close_upvalues(LnVM* vm,Value* last){
    while (vm->open_upvalues != NULL && vm->open_upvalues->value >= last){
        ObjUpvalue* upvalue = vm->open_upvalues;
        upvalue->closed = *upvalue->value;
        upvalue->value = &upvalue->closed;
        vm->open_upvalues = upvalue->next;
    }
}

static void define_method(LnVM* vm, ObjString* name){
    Value method = peek(vm,0);
    ObjClass* klass = AS_CLASS(peek(vm,1));
    table_set(vm,&klass->methods, name,method);
    pop(vm);
}

static void create_class(LnVM* vm,ObjString* name,ObjClass* super_class){
    ObjClass* klass = new_class(vm,name,super_class);
    push(vm, OBJ_VAL(klass));
    if(super_class !=NULL){
        table_add_all(vm,&super_class->methods,&klass->methods);
    }
}

bool is_falsey(Value value){
    return IS_NIL(value) ||
          (IS_BOOL(value) && !AS_BOOL(value)) ||
            (IS_NUMBER(value) && AS_NUMBER(value) == 0) ||
            (IS_STRING(value) && AS_CSTRING(value)[0] == '\0' ) ||
            (IS_LIST(value) && AS_LIST(value)->values.count == 0) ||
            (IS_MAP(value) && AS_MAP(value)->count == 0);
}
static void concatenate(LnVM* vm){
    ObjString* b = AS_STRING(peek(vm,0));
    ObjString* a = AS_STRING(peek(vm,1));

    int length = a->length + b->length;

    char* chars = ALLOCATE(vm,char,length+1);
    memcpy(chars, a->chars,a->length);
    memcpy(chars + a->length,b->chars,b->length);
    chars[length] = '\0';

    ObjString* result = take_string(vm,chars,length);
    pop(vm);
    pop(vm);
    push(vm, OBJ_VAL(result));
}


static LnInterpretResult run(LnVM* vm){
    CallFrame* frame = &vm->frames[vm->frame_count - 1];
    register uint8_t* ip = frame->ip;
#define READ_BYTE() (*ip++)
#define READ_SHORT() \
        (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define READ_CONSTANT() \
        (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
    //TODO: Complete vm
    return INTERPRET_OK;

}