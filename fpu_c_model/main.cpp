#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

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
    // a little shortcut, don't want to build an adder for ~arg + 1;
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
        result[23 + i] = (bool)(exponent & (1 << i));

    unsigned int ui_result = result.pack();
    return *(float*)&ui_result;
}

int convert_float_to_int(float _arg)
{
    bit32 arg(&_arg);

    int exponent = 0;
    for(int i = 0; i < 8; ++i)
        exponent += arg[23 + i] * (1 << i);

    int exp = exponent - 127;

    int result = exp >= 0 ? (1 << exp) : 0;

    for(int i = exp - 1, k = 22; i >= 0 && k >= 0; --i, --k)
        result += arg[k] << i;

    if(arg[31])
        result *= -1;

    return result;
}

float add_float(float _arg1, float _arg2)
{
    bit32 arg1(&_arg1);
    bit32 arg2(&_arg2);

    int exponent1 = 0, exponent2 = 0;;

    for(int i = 0; i < 8; ++i)
    {
        exponent1 += arg1[23 + i] << i;
        exponent2 += arg2[23 + i] << i;
    }

    int exp1 = exponent1 - 127;
    int exp2 = exponent2 - 127;
    int shift_count = exp1 - exp2;

    // swap, arg1 is set to an arg with a bigger exponent
    if(shift_count < 0)
    {
        bit32 tmp = arg1;
        arg1 = arg2;
        arg2 = tmp;
        exp1 = exp2;
        shift_count = -shift_count;
    }

    if(shift_count > 23)
        shift_count = 23;

    // make leading one not implicit
    for(int i = 0; i < 22; ++i)
    {
        arg1[i] = arg1[i + 1];
        arg2[i] = arg2[i + 1];
    }
    arg1[22] = 1;
    arg2[22] = 1;

    // adjust fraction of a smaller exponent number
    for(int i = 0; i < 23 - shift_count; ++i)
        arg2[i] = arg2[i + shift_count];

    for(int i = 0; i < shift_count; ++i)
        arg2[22 - i] = 0;

    // sum, same signs
    if(arg1[31] == arg2[31])
    {
        int carry = 0;

        for(int i = 0; i < 23; ++i)
        {
            int carry_tmp = (arg1[i] & arg2[i]) | (carry & (arg1[i] | arg2[i]));
            arg1[i] = arg1[i] ^ arg2[i] ^ carry;
            carry = carry_tmp;
        }

        if(carry)
        {
            exp1 += 1;
            int exponent = exp1 + 127;

            for(int i = 0; i < 8; ++i)
                arg1[23 + i] = (bool)(exponent & (1 << i));
        }
        else
        {
            for(int i = 22; i > 0; --i)
                arg1[i] = arg1[i - 1];

            arg1[0] = 0;
        }
    }
    // difference
    else
    {
        for(int i = 22; i >= 0; --i)
        {
            if(arg1[i] < arg2[i])
            {
                // we want to subtract from the bigger absolute value
                bit32 tmp = arg1;
                arg1 = arg2;
                arg2 = tmp;
                break;
            }
            else if(arg2[i] < arg1[i])
                break;
        }

        int borrow = 0;

        for(int i = 0; i < 23; ++i)
        {
            int borrow_tmp = (borrow & ~arg1[i]) | (~arg1[i] & arg2[i]) | (arg2[i] & borrow);
            arg1[i] = arg1[i] ^ arg2[i] ^ borrow;
            borrow = borrow_tmp;
        }

        assert(!borrow); // abs(arg1) > abs(arg2)

        int lshift = 0;
        for(int i = 22; i >= 0; --i)
        {
            lshift += 1;

            if(arg1[i])
                break;
        }

        int exponent = 0;

        if(lshift == 23)
            exponent = 0; // exponent must be 0 if the number is 0
        else
            exponent = exp1 - (lshift - 1) + 127; // don't decrease exponent for shifting the implicit one

        // perform shift
        for(int i = 0; i < 23 - lshift; ++i)
            arg1[22 - i] = arg1[22 - i - lshift];

        // clear shifted bits
        for(int i = 0; i < lshift; ++i)
            arg1[i] = 0;

        // set the exponent
        for(int i = 0; i < 8; ++i)
            arg1[23 + i] = (bool)(exponent & (1 << i));
    }

    int packed = arg1.pack();
    return *(float*)&packed;
}

float sub_float(float _arg1, float _arg2)
{
    return {};
}

float mul_float(float _arg1, float _arg2)
{
    return {};
}

float div_float(float _arg1, float _arg2)
{
    return {};
}

// e.g. "1.33434" (base ten)
float string_dec_to_float(const char* str)
{
    assert(str);

    bit32 result;
    result.set_zero();

    if(*str == '-')
    {
        result[31] = 1;
        ++str;
    }

    const char* it = str;

    while(*it && *it != '.')
        ++it;

    const char* frac_it = it;
    if(*frac_it == '.')
        ++frac_it;

    --it;
    int multiplier = 1;
    int int_part = 0;

    while(it >= str)
    {
        int digit = *it - '0';
        int_part += digit * multiplier;
        multiplier *=  10;
        --it;
    }

    it = frac_it;

    while(*it)
        ++it;

    --it;
    multiplier = 1;
    int frac_part = 0;

    while(it >= frac_it)
    {
        int digit = *it - '0';
        frac_part += digit * multiplier;
        multiplier *= 10;
        --it;
    }

    int init_sub_fraction = 5 * multiplier / 10;

    //printf("int part: %d\n", int_part);
    //printf("frac part: %d\n", frac_part);

    int exp = 0;
    bool init = false;
    int frac_pos = 22;

    for(int i = 31; i >= 0; --i)
    {
        if(init)
        {
            result[frac_pos] = (bool)(int_part & (1 << i));
            frac_pos -= 1;
        }
        else if(int_part & (1 << i))
        {
            init = true;
            exp = i;
        }
    }

    // this is to increase some precision, I don't know if it is a good approach
    // it seems that it still loses some precison
    for(;;)
    {
        int tmp = frac_part * 10;
        if(tmp <= frac_part)
            break;
        frac_part = tmp;
        init_sub_fraction *= 10;
    }

    while(frac_part && frac_pos >= 0 && init_sub_fraction)
    {
        int diff = frac_part - init_sub_fraction;
        bool fit = diff >= 0;

        if(init)
        {
            result[frac_pos] = fit;
            frac_pos -= 1;
        }
        else
        {
            exp -= 1;

            if(fit)
                init = true;
        }

        if(fit)
            frac_part -= init_sub_fraction;

        init_sub_fraction /= 2;

    }

    exp += init * 127;

    for(int i = 0; i < 8; ++i)
        result[23 + i] = (bool)(exp & (1 << i));

    int packed_result = result.pack();
    return *(float*)&packed_result;
}

const char* float_to_string_dec(float _arg)
{
    bit32 arg(&_arg);

    int sign = arg[31];
    int exponent = 0;

    for(int i = 0; i < 8; ++i)
        exponent += arg[23 + i] << i;

    int exp = exponent - 127;

    int int_part = 0;
    int frac_part = 0;
    int frac_pos = 22;
    int frac_add = 1000000000; // 10^9

    if(exp >= 0)
    {
        int_part = 1 << exp;
        exp -= 1;
    }
    else
    {
        frac_add = frac_add >> -exp;
        frac_part = frac_add;
    }

    frac_add = frac_add >> 1;

    while(frac_pos >= 0 && exp >= 0)
    {
        int_part += arg[frac_pos] << exp;
        --frac_pos;
        --exp;
    }

    while(frac_pos >= 0 && frac_add > 0)
    {
        frac_part += arg[frac_pos] * frac_add;
        frac_add = frac_add >> 1;
        frac_pos -= 1;
    }

    //printf("int part: %d\n", int_part);
    //printf("frac part: %d\n", frac_part);

    char str_int[1024];
    char str_frac[1024];

    char* str_int_it = str_int;

    do
    {
        *str_int_it = '0' + (int_part % 10);
        int_part /= 10;
        str_int_it += 1;
    }
    while(int_part);

    *str_int_it = 0;

    str_int_it -= 1;
    char* str_int_beg = str_int;

    // reverse
    while(str_int_beg < str_int_it)
    {
        char tmp = *str_int_beg;
        *str_int_beg = *str_int_it;
        *str_int_it = tmp;
        ++str_int_beg;
        --str_int_it;
    }

    char* str_frac_it = str_frac;

    do
    {
        *str_frac_it = '0' + (frac_part % 10);
        frac_part /= 10;
        str_frac_it += 1;
    }
    while(frac_part);

    *str_frac_it = 0;
    str_frac_it -= 1;
    char* str_frac_beg = str_frac;

    // reverse
    while(str_frac_beg < str_frac_it)
    {
        char tmp = *str_frac_beg;
        *str_frac_beg = *str_frac_it;
        *str_frac_it = tmp;
        ++str_frac_beg;
        --str_frac_it;
    }

    char* return_string = (char*)malloc(1000);
    sprintf(return_string, "%s%s.%s", sign ? "-" : "", str_int, str_frac);
    return return_string;
}

void disasm_f32(float _arg)
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
    disasm_f32(55.5f);
    disasm_f32(-0.625111);
    disasm_f32(0);

    printf("%f\n", convert_int_to_float(-666069));

    printf("%d\n", convert_float_to_int(-0.555));
    printf("%d\n", convert_float_to_int(-22.555));
    printf("%d\n", convert_float_to_int(10));
    printf("%d\n", convert_float_to_int(0));

    printf("%f\n", string_dec_to_float("-100.6843"));
    printf("%f\n", string_dec_to_float("-0.225"));
    printf("%f\n", string_dec_to_float("0"));

    printf("%s\n", float_to_string_dec(-5059.99954));
    printf("%s\n", float_to_string_dec(0.f));
    printf("%s\n", float_to_string_dec(6666));
    printf("%s\n", float_to_string_dec(69.69001));

    printf("%f\n", add_float(1.35, 64.9));
    printf("%f\n", add_float(-443.439892, 599.4323211));
    printf("%f\n", add_float(55.55, -55.55));
    printf("%f\n", add_float(-600.3, 200.4));
    printf("%f\n", add_float(55.543, -0.543));

    return 0;
}
