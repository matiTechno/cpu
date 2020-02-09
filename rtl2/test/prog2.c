int mul(int a, int b)
{
    return a * b;
}

int add(int a, int b)
{
    return a + b;
}

int div(int a, int b)
{
    return a / b;
}

int add6(int a, int b, int c, int d, int e, int f)
{
    return a + b + c + d + e + f;
}

int main()
{
    int x = mul(5, 4);
    int y = div(x, 2);
    int z = add(add(x, y), 3);
    return add6(1, 2, 3, 4, 5, 6) + z;
    // 54
}
