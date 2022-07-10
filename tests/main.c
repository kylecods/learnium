#include <stdio.h>
#include <assert.h>

#include "ln.h"

void common_lex_test(char* source){
    Scanner scanner;
    init_scanner(&scanner, source);
    while(true){
      Token token = scan_token(&scanner);

      if((token.type == TOKEN_EOF)) break;

      assert(token.type != TOKEN_ERROR);
    }

}

void lex_test(){
    //keywords
    common_lex_test("while break if continue class func var else import for");

    //operators
    common_lex_test(":,[]{}()><=&;/\"'*-+!|-.");

    //numbers
    common_lex_test("1 90 09 9.0 0x67 0X65 3.3333");

    //single quote string
    common_lex_test("'this is a is single quoted string';");

    //double quote string
    common_lex_test("\"this is a double quoted string\";");
}


int main(){
    lex_test();
    return 0;
}

