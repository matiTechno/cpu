int factorial(int n)
{
    if(n == 0)
        return 1;
    return n * factorial(n - 1);
}

int fib(int n)
{
    if(n == 0)
        return n;
    if(n == 1)
        return n;
    return fib(n - 1) + fib(n - 2);
}

int main()
{
    return factorial(5) * fib(6);
    // 960
}
