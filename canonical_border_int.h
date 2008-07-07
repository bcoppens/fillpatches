#ifndef CANONICAL_BORDER_INT
#define CANONICAL_BORDER_INT

#include <string.h>

#ifndef CanonicalBaseType
#define CanonicalBaseType unsigned long
#endif

#ifndef CanonicalSize
#define CanonicalSize 4
#endif

#define BaseBitSize (8*sizeof(CanonicalBaseType))

struct CanonicalBorderInt {
    CanonicalBaseType content[CanonicalSize]; // most significant content left...
    void clear() {
        memset(this, 0, sizeof(this));
    }

    CanonicalBorderInt() {
        clear();
    }

    CanonicalBorderInt(CanonicalBaseType i) {
        clear();
        content[CanonicalSize - 1] = i;
    }

    CanonicalBorderInt& operator|=(const CanonicalBorderInt& rhs) {
        for (int i = 0; i < CanonicalSize; i++)
            content[i] |= rhs.content[i];
        return *this;
    }

    CanonicalBorderInt& operator=(const CanonicalBorderInt& rhs) {
        for (int i = 0; i < CanonicalSize; i++)
            content[i] = rhs.content[i];
        return *this;
    }

    CanonicalBorderInt operator|(const CanonicalBorderInt& rhs) const {
        CanonicalBorderInt c;
        for (int i = 0; i < CanonicalSize; i++)
            c.content[i] = content[i] | rhs.content[i];
        return c;
    }

    CanonicalBorderInt operator&(const CanonicalBorderInt& rhs) const {
        CanonicalBorderInt c;
        for (int i = 0; i < CanonicalSize; i++)
            c.content[i] = content[i] & rhs.content[i];
        return c;
    }

    CanonicalBorderInt operator<<(const CanonicalBorderInt& rhs) const {
        // ### rhs is actually a BaseType!!!!!
        CanonicalBorderInt c;
        CanonicalBaseType shift = rhs.content[CanonicalSize - 1];
        if (shift % BaseBitSize == 0) {
            // Whole shift:
            for (int i = 0; (i < CanonicalSize) && ((i + (shift / BaseBitSize)) < CanonicalSize); i++) {
                c.content[i] = content[i + (shift / BaseBitSize)];
            }
        } else {
            // [0001][1111][1110] << 9
            // Partial shifts:
            CanonicalBaseType sleft = shift % BaseBitSize;
            CanonicalBaseType sright = BaseBitSize - sleft;
            int idx;
            for (int i = 0; (i < CanonicalSize) && ((i + (shift / BaseBitSize)) < CanonicalSize); i++) {
                idx = i + (shift / BaseBitSize);
                c.content[i] = (content[idx] << sleft) | (content[idx+1] >> sright);
            }
        }

        return c;
    }

    // Slow...
    CanonicalBorderInt& operator<<=(const CanonicalBorderInt& rhs) {
        *this = *this << rhs;
        return *this;
    }
};

#endif // CANONICAL_BORDER_INT
