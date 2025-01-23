#include <stdio.h>
#include "pico/stdlib.h"


int main()
{
    stdio_init_all();

    while (true) {
        // printf("It's MyGO!!!!!\n");
        sleep_ms(100);
    }
}
