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

    
}