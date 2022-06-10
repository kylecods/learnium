#include <stdio.h>
#include <assert.h>

#include "ln.h"

int main(){

    Scanner scanner;

    init_scanner(&scanner, "{");

    Token t = scan_token(&scanner);

    assert(t.type == TOKEN_LEFT_BRACE);

    printf("All tests passed!!");
    return 0;
}