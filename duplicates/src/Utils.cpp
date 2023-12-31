#include <duplicates/Utils.h>

uint16_t digits(size_t n)
{
    uint16_t d = 0;

    while (n != 0)
    {
        d++;
        n /= 10;
    }

    return d;
}
