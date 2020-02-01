struct vec3t
{
    float x;
    float y;
    float z;
};

struct rayt
{
    vec3t origin;
    vec3t direction;
};


int add(int x, int y)
{
    return x + y;
}

float dot(vec3t a, vec3t b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

void main()
{
    // implicit initialization of stack pointer, __malloc() data, etc.

    __send(1);

    // implicit endless loop
}
