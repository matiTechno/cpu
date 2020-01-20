#include <stdio.h>
#include <string.h>

// naming convention
// exponent = exp + bias (127)

struct bit32
{
    bit32() = default;
    bit32(void* _arg)
    {
        unsigned int arg = *(unsigned int*)_arg;
        for(int i = 0; i < 32; ++i)
        {
            data[i] = arg & 1;
            arg = arg >> 1;
        }
    }

    unsigned int pack()
    {
        unsigned int res = 0;
        for(int i = 0; i < 32; ++i)
            res += data[i] << i;
        return res;
    }

    void set_zero()
    {
        memset(data, 0, sizeof(data));
    }

    char data[32];
    char& operator[](int i) {return data[i];}
};

float convert_int_to_float(int _arg)
{
    // a little shortcut, I don't want to implement an adder.
    int sign = _arg < 0;
    if(sign)
        _arg = -_arg;

    bit32 arg(&_arg);

    // find bit positin of a leading 1
    int i;
    for(i = 31; i >= 0; --i)
    {
        if(arg[i])
            break;
    }

    bit32 result;
    result.set_zero();
    result[31] = sign; // set sign bit
    int exponent = i + 127; // correct with bias

    // set fraction
    i -= 1; // skip leading 1
    for(int ri = 22; ri >= 0 && i >= 0; --ri, --i)
        result[ri] = arg[i];

    // set exponent
    for(i = 0; i < 8; ++i)
        result[23 + i] = exponent & (1 << i) ? 1 : 0;

    unsigned int ui_result = result.pack();
    return *(float*)&ui_result;
}

int convert_float_to_int(int _arg)
{
    bit32 arg(&_arg);
    return {};
}

float add_float(int _arg1, int _arg2)
{
    bit32 arg1(&_arg1);
    bit32 arg2(&_arg2);
    return {};
}

float sub_float(int _arg1, int _arg2)
{
    return {};
}

float mul_float(int _arg1, int _arg2)
{
    return {};
}

float div_float(int _arg1, int _arg2)
{
    return {};
}

void print_f32(float _arg)
{
    printf("disassembly of: %f\n", _arg);
    bit32 arg(&_arg);

    printf("arg[31:0]: ");

    for(int i = 31; i > -1; --i)
        printf("%d", arg[i]);
    printf("\n");

    int exponent = 0;
    for(int i = 0; i < 8; ++i)
        exponent += arg[23 + i] << i;

    printf("sign: %d\nexponent (with bias added): %d\nfraction: ", arg[31], exponent);
    for(int i = 31; i > 23; --i)
        printf("%d", arg[i]);

    printf("\n");

    printf("normalized form: %c1.", arg[31] ? '-' : '+');
    for(int i = 22; i > -1; --i)
        printf("%d", arg[i]);

    int exp = exponent - 127;

    printf(" * 2^%d\n", exp);

    int int_part = (exp < 0) ? 0 : 1 << exp;

    for(int i = 0; i < exp; ++i)
        int_part += arg[22 - i] << ((-1 - i) + exp);

    printf("integer part: %d\n\n", int_part);
}

int main()
{
    print_f32(55.5f);
    print_f32(-0.625111);
    print_f32(0);

    printf("%f\n", convert_int_to_float(-666069));

    return 0;
}
