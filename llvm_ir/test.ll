;declare i32 @factorial(i32)

define i32 @factorial(i32 %val)
{
    %is_base_case = icmp eq i32 %val, 0
    br i1 %is_base_case, label %base_case, label %recursive_case
base_case:
    ret i32 1
recursive_case:
    %1 = add i32 -1, %val
    %2 = call i32 @factorial(i32 %1)
    %3 = mul i32 %val, %2
    ret i32 %3
}

define i32 @fib(i32 %val)
{
    %1 = icmp slt i32 %val, 2
    br i1 %1, label %then_case, label %else_case
then_case:
    ret i32 %val
else_case:
    %tmp = sub i32 %val, 1
    %2 = call i32 @fib(i32 %tmp)
    %3 = sub i32 %tmp, 1
    %4 = call i32 @fib(i32 %3)
    %5 = add i32 %2, %4
    ret i32 %5
}

declare i32 @printf(i8*, ...)

@fmt = constant [4 x i8] c"%d\0A\00"

define i32 @main(i32 %argc, i8** %argv)
{
    %1 = call i32 @factorial(i32 10)
    %2 = mul i32 %1, 7
    %3 = icmp eq i32 %2, 42
    %result = zext i1 %3 to i32
    %4 = getelementptr [4 x i8], [4 x i8]* @fmt, i32 0, i32 0
    call i32 (i8*, ...) @printf(i8* %4, i32 %1)
    %6 = call i32 @fib(i32 8)
    call i32 (i8*, ...) @printf(i8* %4, i32 %6)
    ret i32 %result
}
