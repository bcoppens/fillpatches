#include <stdio.h>
#include "external_c_interface.h"

static const char* b1 = "232223232223";
static const char* b2 = "223223223223";


int main() {
    printf("'%s' has %i fillings\n", b1, number_of_fillings(b1));
    printf("'%s' has %i fillings\n", b2, number_of_fillings(b2));
    return 0;
}

