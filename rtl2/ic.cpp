#include <stdarg.h>
#include <stdio.h>
#include "ic.h"

void ic_exit(int line, int col, const char* fmt, ...)
{
    printf("error: line %d, col %d: ", line, col);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    exit(1);
}

bool ic_strcmp(ic_str lhs, ic_str rhs)
{
    if(lhs.len != rhs.len)
        return false;

    for(int i = 0; i < lhs.len; ++i)
    {
        if(lhs.data[i] != rhs.data[i])
            return false;
    }
    return true;
}


#define IC_MAX_STRUCT 100

int main(int argc, const char** argv)
{
    if(argc != 2)
    {
        printf("expected a file name argument\n");
        return 1;
    }

    FILE* file = fopen(argv[1], "rb");

    if(!file)
    {
        printf("could not open a file\n");
        return 1;
    }

    int rc = fseek(file, 0, SEEK_END);
    assert(!rc);
    int file_size = ftell(file);
    assert(file_size != -1);
    rewind(file);
    char* file_data = (char*)malloc(file_size + 1);
    fread(file_data, 1, file_size, file);
    fclose(file);
    file_data[file_size] = '\0';
    ic_token* tokens = lex(file_data);
    array<ic_function> functions;
    array<ic_struct> structures;
    functions.init();
    structures.init();
    structures.resize(IC_MAX_STRUCT);
    structures.size = 0;
    parse(tokens, functions, structures);
    assert(structures.size <= IC_MAX_STRUCT); // pointers to structures data must not be invalidated, see ic_type
    sema(functions, structures);
    //gen_llvm(functions, structures);
    gen_mycore(functions, structures);
    return 0;
}
