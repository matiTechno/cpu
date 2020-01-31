#pragma once
#include <stdlib.h>

template<typename T>
struct array
{
    int size;
    int capacity;
    T* buf;

    void init()
    {
        size = 0;
        capacity = 0;
        buf = nullptr;
        // give it a start
        resize(20);
        size = 0;
    }

    T* transfer()
    {
        T* temp = buf;
        buf = nullptr;
        return temp;
    }

    void free()
    {
        ::free(buf);
    }

    void clear()
    {
        size = 0;
    }

    void resize(int new_size)
    {
        size = new_size;
        _maybe_grow();
    }

    void push_back()
    {
        size += 1;
        _maybe_grow();
    }

    void push_back(T t)
    {
        size += 1;
        _maybe_grow();
        back() = t;
    }

    void pop_back()
    {
        size -= 1;
    }

    T& back()
    {
        return *(buf + size - 1);
    }

    T* begin()
    {
        return buf;
    }

    T* end()
    {
        return buf + size;
    }

    T& operator[](int i)
    {
        return buf[i];
    }

    void _maybe_grow()
    {
        if (size > capacity)
        {
            capacity = size * 2;
            buf = (T*)realloc(buf, capacity * sizeof(T));
            assert(buf);
        }
    }
};
