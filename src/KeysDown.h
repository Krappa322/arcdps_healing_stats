#pragma once

class KeysDown
{
public:
    static void SetKeyDown(int key, bool down)
    {
        keys[key] = down;
    }

    static bool IsKeyDown(int key)
    {
        return keys[key];
    }

    static size_t Size()
    {
        return sizeof(keys);
    }
private:
    inline static bool keys[512] = {};
};
