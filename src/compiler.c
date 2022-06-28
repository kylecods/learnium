#include "ln.h"


static void parser_init(Parser *parser){
    parser->hasError = false;
    parser->panicMode = false;

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

    //TODO vm;

    compiler->function = new_function(parser->vm,parser->module);//TO add VM

    switch (type)
    {
        case TYPE_FUNCTION:
            compiler->function->name = NULL;//TODO:hashtable
            break;
        
        default:
            break;
    }

    Local* local = &compiler->locals[compiler->local_count++];

    local->depth = compiler->scope_depth;
    local->is_captured = false;

    if(type != TYPE_METHOD) {
        local->name.start = "this";
        local->name.length = 4;
    } else{
        local->name.start = "";
        local->name.length = 0;

    }
}

static ObjFun* end_compiler(Compiler* compiler){//TODO:emit}
}
static void begin_scope(Compiler* compiler){
    compiler->scope_depth++;
}

static void end_scope(Compiler* compiler){
    compiler->scope_depth--;

    while (compiler->local_count >0 && compiler->locals[compiler->local_count - 1].depth >
    compiler->scope_depth)
    {
        //TODO:vm;
        compiler->local_count--;
    }
    
}

static void parse_precedence(Compiler* compiler, Precedence precedence);
static void expression(Compiler* compiler);
static void statement(Compiler* compiler);
static void declaration(Compiler* compiler);
static ParserRule* get_rule(TokenType type);

static uint8_t identifier_constant(Compiler* compiler, Token* name){}

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
        //TODO:VM
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
    //TODO:VM
    parse_precedence(compiler,PREC_AND);
}