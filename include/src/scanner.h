#ifndef file_scanner_h
#define file_scanner_h

typedef enum{
    TOKEN_EOF = 0,
    TOKEN_ERROR,
    TOKEN_LINE,

    //keywords
    TOKEN_WHILE,TOKEN_FOR,TOKEN_FUNC,TOKEN_IF,
    TOKEN_ELSE,TOKEN_RETURN,TOKEN_CONTINUE,TOKEN_VAR,
    TOKEN_CLASS,TOKEN_BREAK,TOKEN_IMPORT,

    //single character tokens
    TOKEN_PLUS,TOKEN_MINUS,TOKEN_SLASH,TOKEN_STAR,
    TOKEN_DOT,TOKEN_EQUALS,TOKEN_LEFT_PAREN,TOKEN_RIGHT_PAREN,
    TOKEN_SEMICOLON,TOKEN_FULL_COLON,TOKEN_COMMA,TOKEN_RIGHT_BRACKET,
    TOKEN_LEFT_BRACKET,TOKEN_BANG,TOKEN_LESS,TOKEN_GREATER,
    TOKEN_LEFT_BRACE,TOKEN_RIGHT_BRACE,TOKEN_AMP,
    
    
    TOKEN_PIPE,TOKEN_SHIFT_RIGHT,TOKEN_SHIFT_LEFT,

    //double character tokens
    TOKEN_PLUSEQ,TOKEN_MINUSEQ,TOKEN_STAREQ,TOKEN_SLASHEQ,
    TOKEN_LESSEQ,TOKEN_GREATEREQ,TOKEN_EQUALEQ,TOKEN_BANGEQ,
    TOKEN_PIPEPIPE,TOKEN_AMPAMP,

    TOKEN_STRING,TOKEN_NUM,TOKEN_IDENTIFIER,

}TokenType;


typedef struct 
{
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

typedef struct 
{
    const char* start;
    const char* current;
    int line;

} Scanner;

typedef struct{
    const char* identifier;
    int length;
    TokenType type;
} Keyword;

void init_scanner(Scanner* scanner, const char* source);

Token scan_token(Scanner* scanner);

#endif