#include <cassert>
#include <iostream>

// 64 bit only test!

#define CanonicalBaseType unsigned char
#define CanonicalSize 4

#include "canonical_border_int.h"

using namespace std;

void testShl() {
    for (unsigned int i = 0; i < 0xffff; i++) {
        CanonicalBorderInt J = CanonicalBorderInt(i & 0xff);
        assert(J.content[3] == (i & 0xff));
        J |= CanonicalBorderInt(i >> 8) << CanonicalBorderInt(8);
        assert(J.content[3] == (i & 0xff));
        assert(J.content[2] == ((i >> 8) & 0xff));

        for (unsigned int k = 0; k < 32; k++) {
            unsigned int j = i << k;
            J <<= CanonicalBorderInt(k);
            assert((0xff & (j)) == J.content[3]);
            assert((0xff & (j>>8)) == J.content[2]);
            assert((0xff & (j>>16)) == J.content[1]);
            assert((0xff & (j>>24)) == J.content[0]);
        }
    }
}


int main() {
    testShl();
}
