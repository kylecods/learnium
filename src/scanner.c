#include "ln.h"


void init_scanner(Scanner* scanner, const char* source){
    scanner->current = source;

    scanner->start = source;

    scanner->line = 1;
}

static bool is_alpha(char c){
  return (c >= 'a' && c <= 'z')||
         (c >= 'A' && c <= 'Z')||
          c == '_';
}

static bool is_digit(char c){
    return c >= '0' && c <= '9';
}

static bool is_hex(char c){
    return ((c>= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') || (c == '_'));
}

static bool is_end(Scanner *scanner){
    return *scanner->current == '\0';
}

static char advance(Scanner* scanner){
    scanner->current++;
    return scanner->current[-1];
}

static char peek_scanner(Scanner* scanner){
    return *scanner->current;
}

static char peek_next(Scanner* scanner){
    if(is_end(scanner)) return '\0';

    return scanner->current[1];
}

static bool match(Scanner* scanner, char expected){
    if(is_end(scanner)) return false;

    if(*scanner->current != expected) return false;

    scanner->current++;
    return true;
}

static Token create_token(Scanner* scanner,TokenType type){
    Token token;
    token.start = scanner->start;
    token.length = (int) (scanner->current - scanner->start);
    token.line = scanner->line;
    token.type = type;

    return token;
}

static Token error_token(Scanner* scanner,const char* message){
    Token token;
    token.start = message;
    token.length = (int) strlen(message);
    token.line = scanner->line;
    token.type = TOKEN_ERROR;

    return token;
}

static void skip_whitespace(Scanner* scanner){
    while(true){
        char c = peek_scanner(scanner);

        switch (c)
        {
        case ' ':
        case '\r':
        case '\t':
            advance(scanner);
            break;
        case '\n':
            scanner->line++;
            advance(scanner);
            break;
        case '/':
            if(peek_next(scanner) == '*'){
                //mutiline comments
                advance(scanner);
                advance(scanner);
                while (true)
                {
                    while (peek_next(scanner) != '*' && !is_end(scanner))
                    {
                        if((c = advance(scanner)) == '\n') scanner->line++;
                    }

                    if(is_end(scanner)) return;

                    if (peek_next(scanner) == '/') break;

                    advance(scanner);

                }
                advance(scanner);
                advance(scanner);
            }else if(peek_next(scanner) == '/'){
                while (peek_scanner(scanner) != '\n' && !is_end(scanner)){
                    advance(scanner);
                }
                
            }else{
                return;
            }
            break;
        default:
            return;
        }
    }
}

static Keyword keywords[] =
{
    {"while", 5, TOKEN_WHILE},
    {"if", 5, TOKEN_IF},
    {"func", 5, TOKEN_FUNC},
    {"for", 5, TOKEN_FOR},
    {"return", 5, TOKEN_RETURN},
    {"else", 5, TOKEN_ELSE},
    {"var", 5, TOKEN_VAR},
    {"class", 5, TOKEN_CLASS},
    {"break", 5, TOKEN_BREAK},
    {"continue", 5, TOKEN_CONTINUE},
    {"import", 5, TOKEN_IMPORT},
    {"NULL", 0, TOKEN_EOF}//sentinel
};

static Token identifier(Scanner* scanner){
    while (is_alpha(peek_scanner(scanner)) || is_digit(peek_scanner(scanner)))
    {
        advance(scanner);
    }

    const char* name_start = scanner->start;

    TokenType type = TOKEN_IDENTIFIER;
    int length = (int)(scanner->current - scanner->start);
    for (int i = 0; keywords[i].identifier != NULL; i++)
    {
        if(length == keywords[i].length && memcmp(name_start,keywords[i].identifier, length) == 0){
            type = keywords[i].type;
            break;
        }
    }
    
    return create_token(scanner, type);
}

static Token number(Scanner* scanner){
    while (is_digit(peek_scanner(scanner)) || peek_scanner(scanner) == '_')
    {
        advance(scanner);
    }
    if(peek_scanner(scanner) == '.' && (is_digit(peek_next(scanner)))){
        advance(scanner);
        while (is_digit(peek_scanner(scanner)) || peek_scanner(scanner) == '_')
        {
            advance(scanner);
        }
        
    }
    return create_token(scanner,TOKEN_NUM);
}

static Token hex_number(Scanner* scanner){
    while (peek_scanner(scanner) == '_') advance(scanner);
    if(peek_scanner(scanner) == '0') advance(scanner);

    if(peek_scanner(scanner) == 'x' || peek_scanner(scanner) == 'X'){
        advance(scanner);
        if(!is_hex(peek_scanner(scanner))) return error_token(scanner, "Invalid hex literal");

        while (is_hex(peek_scanner(scanner)))
        {
            advance(scanner);
        }
        return create_token(scanner,TOKEN_NUM);
    }else return number(scanner);
}

static Token string(Scanner* scanner, char string_token){
    while(peek_scanner(scanner) == string_token && !is_end(scanner)){
        advance(scanner);
        if(peek_scanner(scanner) == '\n') scanner->line++;
    }
    if(is_end(scanner)) return error_token(scanner, "Unterminated string");

    advance(scanner);

    return create_token(scanner, TOKEN_STRING);
}

Token scan_token(Scanner* scanner){
    skip_whitespace(scanner);

    scanner->start = scanner->current;

    if(is_end(scanner)) return create_token(scanner, TOKEN_EOF);

    char c = advance(scanner);

    if(is_alpha(c)) return identifier(scanner);
    if(is_digit(c)) return hex_number(scanner);

    
  switch(c){
    case '(': return create_token(scanner,TOKEN_LEFT_PAREN);
    case ')': return create_token(scanner,TOKEN_RIGHT_PAREN);
    case '{': return create_token(scanner,TOKEN_LEFT_BRACE);
    case '}': return create_token(scanner,TOKEN_RIGHT_BRACE);
    case '[': return create_token(scanner,TOKEN_LEFT_BRACKET);
    case ']': return create_token(scanner,TOKEN_RIGHT_BRACKET);
    case ';': return create_token(scanner,TOKEN_SEMICOLON);
    case ',': return create_token(scanner,TOKEN_COMMA);
    case '.': return create_token(scanner,TOKEN_DOT);
    case '-': return create_token(scanner,TOKEN_MINUS);
    case '+': return create_token(scanner,TOKEN_PLUS);
    case '/':
        if(match(scanner, '=')){
            return create_token(scanner,TOKEN_SLASHEQ);
        } else { 
            return create_token(scanner,TOKEN_SLASH);
        }
    case '*': 
        if(match(scanner, '=')){
            return create_token(scanner, TOKEN_STAREQ);
        }
        else {
            return create_token(scanner,TOKEN_STAR);
        }
    case '|': 
        if(match(scanner, '|')){
            return create_token(scanner,TOKEN_PIPEPIPE);
        }else{
            return create_token(scanner,TOKEN_PIPE);
        }

    case '&': return create_token(scanner,match(scanner,'&') ? TOKEN_AMPAMP :TOKEN_AMP);
    case '!': return create_token(scanner,match(scanner, '=') ? TOKEN_BANGEQ : TOKEN_BANG);
    case '=': return create_token(scanner,match(scanner, '=') ? TOKEN_EQUALEQ : TOKEN_EQUALS);
    case '<': {
        if (match(scanner, '=')) {
            return create_token(scanner,TOKEN_LESSEQ);
        } else if (match(scanner,'<')) {
            return create_token(scanner,TOKEN_SHIFT_LEFT);
        } else {
            return create_token(scanner,TOKEN_LESS);
        }
    }
    case '>': {
        if (match(scanner,'=')) {
            return create_token(scanner,TOKEN_GREATEREQ);
        } else if (match(scanner,'>')) {
            return create_token(scanner,TOKEN_SHIFT_RIGHT);
        } else {
            return create_token(scanner,TOKEN_GREATER);
        }

    }
    case '"': return string(scanner,'"');
    case '\'': return string(scanner, '\'');
    case ':': return create_token(scanner, TOKEN_FULL_COLON);
  }
  return error_token(scanner, "Unexpected character");
}