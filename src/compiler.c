#include "ln.h"


static void parser_init(Parser *parser){
    parser->hasError = false;
    parser->panicMode = false;

}
static Chunk* current_chunk(Compiler* compiler){
    return &compiler->function->chunk;
}
static void error_at(Parser* parser, Token* token, const char* message){
    if(parser->panicMode) return;

    parser->panicMode = true;

    fprintf(stderr, "[line %d] ERROR", token->line);

    if(token->type == TOKEN_EOF){
        fprintf(stderr, " at end");
    }else if (token->type == TOKEN_ERROR)
    {
        /* code */
    }else{
        fprintf(stderr, " at '%.*s'\n", token->length, token->start);
    }
    fprintf(stderr,": %s\n", message);
    parser->hasError = true;
}

static void error(Parser* parser,const char* message){
    error_at(parser, &parser->previous, message);
}

static void error_at_current(Parser* parser,const char* message){
    error_at(parser,&parser->current,message);
}

static void advance(Parser* parser){
    parser->previous = parser->current;

    for(;;){
        parser->current = scan_token(&parser->scanner);
        if(parser->current.type != TOKEN_ERROR) break;

        error_at(parser,&parser->current,parser->current.start);
    }
}

static void consume(Compiler* compiler,TokenType type, const char* message){
    if(compiler->parser->current.type == type){
        advance(compiler->parser);
        return;
    }
    error_at_current(compiler->parser,message);
}

static bool check(Compiler* compiler, TokenType type){
    return compiler->parser->current.type == type;
}

static bool match(Compiler* compiler, TokenType type){
    if(!check(compiler,type)) return false;

    advance(compiler->parser);
    return true;
}
static void emit_byte(Compiler* compiler, uint8_t byte){
    write_chunk(compiler->parser->vm, current_chunk(compiler),byte,compiler->parser->previous.line);
}
static void emit_bytes(Compiler* compiler,uint8_t first_byte, uint8_t second_byte){
    emit_byte(compiler,first_byte);
    emit_byte(compiler,second_byte);
}

static void emit_loop(Compiler* compiler, int loop_start){
    emit_byte(compiler,OP_LOOP);
    int offset = current_chunk(compiler)->count - loop_start + 2;
    if(offset > UINT16_MAX) error(compiler->parser, "Loop body too large");

    emit_byte(compiler,(offset >> 8) & 0xff);
    emit_byte(compiler,offset & 0xff);
}

static int emit_jump(Compiler* compiler, uint8_t instruction){
    emit_byte(compiler,instruction);
    emit_byte(compiler,0xff);
    emit_byte(compiler,0xff);
    return current_chunk(compiler)->count - 2;
}
static void emit_return(Compiler* compiler){
    if(compiler->type == TYPE_INITIALIZER){
        emit_bytes(compiler,OP_GET_LOCAL,0);
    } else{
        emit_byte(compiler,OP_NIL);
    }
    emit_byte(compiler,OP_RETURN);
}

static uint8_t make_constant(Compiler* compiler, Value value){
    int constant = add_constant(compiler->parser->vm, current_chunk(compiler),value);
    if(constant > UINT8_MAX){
        error(compiler->parser, "Too many constants in one chunk");
        return 0;
    }
    return (uint8_t)constant;
}
static void emit_constant(Compiler* compiler, Value value){
    emit_bytes(compiler,OP_CONSTANT, make_constant(compiler,value));
}
static void patch_jump(Compiler* compiler, int offset){
    int jump = current_chunk(compiler)->count - offset - 2;

    if (jump > UINT16_MAX){
        error(compiler->parser, "Too much code to jump over");
    }

    current_chunk(compiler)->code[offset] = (jump >> 8) & 0xff;
    current_chunk(compiler)->code[offset + 1] = jump & 0xff;
}

static void init_compiler(Parser* parser, Compiler* compiler, Compiler* parent, FunctionType type){
    compiler->parser = parser;
    compiler->enclosing = parent;
    compiler->function = NULL; 
    compiler->class = NULL;

    if(parent != NULL){
        compiler->class = parent->class;
        compiler->loop = parent->loop;
    }

    compiler->type = type;
    compiler->local_count = 0;
    compiler->scope_depth = 0;

    parser->vm->compiler = compiler;

    compiler->function = new_function(parser->vm, parser->module);

    compiler->function = new_function(parser->vm,parser->module);//TO add VM

    switch (type)
    {
        case TYPE_INITIALIZER:
        case TYPE_FUNCTION:
        case TYPE_METHOD:
            compiler->function->name = copy_string(parser->vm,parser->previous.start,parser->previous.length);
            break;
        default:
            break;
    }

    Local* local = &compiler->locals[compiler->local_count++];

    local->depth = compiler->scope_depth;
    local->is_captured = false;
    if(type == TYPE_METHOD || type == TYPE_INITIALIZER) {
        local->name.start = "this";
        local->name.length = 4;
    } else{
        local->name.start = "";
        local->name.length = 0;

    }
}

static ObjFun* end_compiler(Compiler* compiler){
    emit_return(compiler);
    ObjFun* function = compiler->function;
    //TODO: DEBUGGER
    if(compiler->enclosing != NULL){
        emit_bytes(compiler->enclosing,OP_CLOSURE, make_constant(compiler->enclosing, OBJ_VAL(function)));

        for (int i = 0; i< function->upvalue_count; i++){
            emit_byte(compiler->enclosing, compiler->upvalues[i].is_local ? 1: 0);
            emit_byte(compiler->enclosing, compiler->upvalues[i].index);
        }
    }
    compiler->parser->vm->compiler = compiler->enclosing;
    return function;

}
static void begin_scope(Compiler* compiler){
    compiler->scope_depth++;
}

static void end_scope(Compiler* compiler){
    compiler->scope_depth--;

    while (compiler->local_count >0 && compiler->locals[compiler->local_count - 1].depth >
    compiler->scope_depth)
    {
        if(compiler->locals[compiler->local_count - 1].is_captured){
            emit_byte(compiler,OP_CLOSE_UPVALUE);
        } else{
            emit_byte(compiler,OP_POP);
        }
        compiler->local_count--;
    }
    
}

static void parse_precedence(Compiler* compiler, Precedence precedence);
static void expression(Compiler* compiler);
static void statement(Compiler* compiler);
static void declaration(Compiler* compiler);
static ParserRule* get_rule(TokenType type);

static uint8_t identifier_constant(Compiler* compiler, Token* name) {
    ObjString *string = copy_string(compiler->parser->vm, name->start, name->length);

    uint8_t index = make_constant(compiler, OBJ_VAL(string));
    return index;
}


static bool identifiers_equal(Token* a, Token* b){
    if(a->length != b->length) return false;

    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(Compiler* compiler, Token* name){
    for (int i = compiler->local_count - 1; i >= 0; i--)
    {
        Local* local = &compiler->locals[i];
        if(identifiers_equal(name, &local->name)){
            if(local->depth == -1){
                error(compiler->parser, "Cannot read local variable in its own initializer");
            }
            return i;
        }
    }
    return -1;
}

static int add_upvalue(Compiler* compiler,uint8_t index, bool is_local){
    int upvalue_count = compiler->function->upvalue_count;
    for (int i = 0; i < upvalue_count; i++)
    {
        UpValue* upvalue = &compiler->upvalues[i];
        if(upvalue->index == index && upvalue->is_local == is_local){
            return i;
        }
    }

    if(upvalue_count == UINT8_COUNT){
        error(compiler->parser,"Too many closure variables in function");
        return 0;
    }

    compiler->upvalues[upvalue_count].is_local = is_local;
    compiler->upvalues[upvalue_count].index = index;

    return compiler->function->upvalue_count++;
    
}

static int resolve_upvalue(Compiler* compiler,Token* name){
    if(compiler->enclosing == NULL) return -1;

    int local = resolve_local((Compiler *) compiler->enclosing, name);

    if(local != -1){
        compiler->enclosing->locals[local].is_captured = true;
        return add_upvalue(compiler,(uint8_t)local,true);
    }

    int up_value = resolve_upvalue(compiler->enclosing,name);

    if(up_value != -1){
        return add_upvalue(compiler,(uint8_t)local,false);
    }

    return -1;
}

static void add_local(Compiler* compiler, Token name){
    if(compiler->local_count == UINT8_COUNT){
        error(compiler->parser, "Too many local variables in function");
        return;
    }

    Local* local = &compiler->locals[compiler->local_count];
    local->name = name;

    local->depth = -1;
    local->is_captured = false;
    compiler->local_count++;
}

static void declare_variable(Compiler* compiler, Token* name){
    if(compiler->scope_depth == 0) return;

    for (int i = compiler->local_count - 1; i >= 0; i--)
    {
        Local* local = &compiler->locals[i];
        if(local->depth != -1 && local->depth < compiler->scope_depth) break;

        if(identifiers_equal(name,&local->name)){
            error_at(compiler->parser, name, "Variable with this name already declared in the scope");
        }
    }

    add_local(compiler, *name);
    
}

static uint8_t parse_variable(Compiler* compiler, const char* error_message){
    consume(compiler,TOKEN_IDENTIFIER, error_message);

    if(compiler->scope_depth == 0){
        return identifier_constant(compiler,&compiler->parser->previous);    }
    declare_variable(compiler,&compiler->parser->previous);

    return 0;
}

static void define_variable(Compiler* compiler, uint8_t global){
    if(compiler->scope_depth == 0){
        emit_bytes(compiler,OP_DEFINE_MODULE,global);
    }else{
        compiler->locals[compiler->local_count - 1].depth = compiler->scope_depth;
    }
}

static int argument_list(Compiler* compiler){
    int arg_count = 0;
    if(!check(compiler, TOKEN_RIGHT_PAREN)){

        if(match(compiler, TOKEN_LEFT_BRACE)){
            do{
                expression(compiler);
                arg_count++;

                if(arg_count > 255){
                    error(compiler->parser, "Cannot have more than 255 arguments");
                }
            }while (match(compiler,TOKEN_COMMA));

            consume(compiler,TOKEN_RIGHT_BRACE,"Expected to close with '}' ");
        }else{
            do{
                expression(compiler);
                arg_count++;

                if(arg_count > 255){
                    error(compiler->parser, "Cannot have more than 255 arguments");
                }
            }while (match(compiler,TOKEN_COMMA));
        }

        
    }
    consume(compiler,TOKEN_RIGHT_PAREN,"Expected ')' after arguments");

    return arg_count;
}

static void expression(Compiler* compiler){

}
static void parse_precedence(Compiler* compiler, Precedence precedence){

}

static void and_(Compiler* compiler, bool can_assign){
    int end_jump = emit_jump(compiler,OP_JUMP_IF_FALSE);
    emit_byte(compiler,OP_POP);
    parse_precedence(compiler,PREC_AND);
    patch_jump(compiler,end_jump);
}

static bool fold_binary(Compiler* compiler,TokenType operator_type){
#define FOLD(operator) \
    do{                \
        Chunk* chunk = current_chunk(compiler); \
        uint8_t index = chunk->code[chunk->count - 1]; \
        uint8_t constant = chunk->code[chunk->count - 3]; \
        if(chunk->code[chunk->count - 2] != OP_CONSTANT) return false; \
        if(chunk->code[chunk->count - 4] != OP_CONSTANT) return false; \
        chunk->constants.value[constant] = NUMBER_VAL(AS_NUMBER(chunk->constants.value[constant]) operator AS_NUMBER(chunk->constants.value[index]));\
        chunk->constants.count--;               \
        chunk->count -=2;                       \
        return true;\
    }while(false)

#define FOLD_FUNC(func) \
        do{             \
            Chunk* chunk = current_chunk(compiler); \
            uint8_t index = chunk->code[chunk->count - 1]; \
            uint8_t constant = chunk->code[chunk->count - 3]; \
            if(chunk->code[chunk->count-2] != OP_CONSTANT) return false; \
            if(chunk->code[chunk->count - 4] != OP_CONSTANT) return false; \
            chunk->constants.values[constant] = NUMBER_VAL(\
            func(\
                AS_NUMBER(chunk->constants.value[constant]),  \
                AS_NUMBER(chunk->constants.value[index])                        \
              )           \
            );          \
            chunk->constants.count--;               \
            chunk->count -= 2;                      \
            return true;\
        }while(false)

    switch (operator_type) {
        case TOKEN_PLUS:{
            FOLD(+);
            return false;
        }
        case TOKEN_MINUS:{
            FOLD(-);
            return false;
        }
        case TOKEN_STAR:{
            FOLD(*);
            return false;
        }
        case TOKEN_SLASH:{
            FOLD(/);
            return false;
        }
        default: return false;
    }
#undef FOLD
#undef FOLD_FUNC
}

static void binary(Compiler* compiler, Token previous_token, bool can_assign) {
    TokenType operator_type = compiler->parser->previous.type;

    ParserRule *rule = get_rule(operator_type);
    parse_precedence(compiler, (Precedence) (rule->precedence + 1));

    TokenType current_token = compiler->parser->previous.type;

    //constant fold optimization
    if ((previous_token.type == TOKEN_NUM) && (current_token == TOKEN_NUM || current_token == TOKEN_LEFT_PAREN) &&
        fold_binary(compiler, operator_type)) {
        return;
    }

    switch (operator_type) {
        case TOKEN_BANGEQ:
            emit_bytes(compiler, OP_EQUAL, OP_NOT);
            break;
        case TOKEN_EQUALEQ:
            emit_byte(compiler, OP_EQUAL);
            break;
        case TOKEN_GREATER:
            emit_byte(compiler, OP_GREATER);
            break;
        case TOKEN_GREATEREQ:
            emit_bytes(compiler, OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emit_byte(compiler, OP_LESS);
            break;
        case TOKEN_LESSEQ:
            emit_bytes(compiler, OP_GREATER, OP_NOT);
            break;
        case TOKEN_PLUS:
            emit_byte(compiler, OP_ADD);
            break;
        case TOKEN_MINUS:
            emit_byte(compiler, OP_SUB);
            break;
        case TOKEN_STAR:
            emit_byte(compiler, OP_MUL);
            break;
        case TOKEN_SLASH:
            emit_byte(compiler, OP_DIV);
            break;
        case TOKEN_AMP:
            emit_byte(compiler, OP_BITWISE_AND);
            break;
        case TOKEN_CARET:
            emit_byte(compiler, OP_BITWISE_XOR);
            break;
        case TOKEN_PIPE:
            emit_byte(compiler, OP_BITWISE_OR);
            break;
        default:
            return;
            //TODO: add powers e,g 10^2 = 20
    }
}

static void ternary(Compiler *compiler, Token previous_token, bool can_assign){
    int else_jump = emit_jump(compiler,OP_JUMP_IF_FALSE);
    emit_byte(compiler,OP_POP);
    expression(compiler);

    int end_jump = emit_jump(compiler,OP_JUMP);
    patch_jump(compiler,else_jump);
    emit_byte(compiler,OP_POP);
    consume(compiler,TOKEN_FULL_COLON, "Expected colon after ternary");
    expression(compiler);
    patch_jump(compiler,end_jump);
}

static void call(Compiler* compiler, Token previous_token, bool can_assign){
    int arg_count = argument_list(compiler);
    emit_bytes(compiler,OP_CALL,arg_count);
}
