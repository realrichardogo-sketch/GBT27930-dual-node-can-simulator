#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    if (puts("Bootstrap smoke test passed.") == EOF) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
