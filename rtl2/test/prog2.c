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

int main()
{
    int x = mul(5, 4);
    int y = div(x, 2);
    return add(add(x, y), 3);
}
