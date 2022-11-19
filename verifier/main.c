// https://www.quut.com/c/ANSI-C-grammar-l-1999.html
// https://www.quut.com/c/ANSI-C-grammar-y-1999.html
// Used to generate expected results for test files
#include <stdio.h>

void yyset_in(FILE*);
void yyset_out(FILE*);
int yylex();
char* yyget_text();

int main(int argc, char** argv) {
    if (argc != 3)
        return 1;
    FILE* fptr = fopen(argv[1], "r");
    FILE* optr = fopen(argv[2], "w");
    yyset_in(fptr);
    yyset_out(NULL);
    int tok = 0;
    while (tok = yylex())
        fprintf(optr, "%s %d\n", yyget_text(), tok);
    fclose(fptr);
    fclose(optr);
    return 0;
}
