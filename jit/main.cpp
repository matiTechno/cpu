#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>

using my_func_t = int(*)(int);

int test(int arg)
{
    return arg + 5;
}

int main()
{
    my_func_t fun =  test;
    int result = fun(3);
    printf("result: %d\n", result);


    unsigned int* exe = (unsigned int*)mmap(nullptr, 2 * sizeof(int), PROT_EXEC | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    assert(exe);

    exe[0] = 0x05c78348;
    exe[1] = 0xc3f88948;

    fun = (my_func_t)exe;

    result = fun(11);

    printf("result: %d\n", result);

    return 0;
}
