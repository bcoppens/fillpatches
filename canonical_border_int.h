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
        memset(this, 0, sizeof(*this));
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
            int idx, i;
            for (i = 0; (i < CanonicalSize) && ((i + (shift / BaseBitSize)) < CanonicalSize); i++) {
                idx = i + (shift / BaseBitSize);
                if (idx + 1 < CanonicalSize)
                    c.content[i] = (content[idx] << sleft) | (content[idx+1] >> sright);
                else // Final round:
                    c.content[i] = (content[idx] << sleft);
            }
        }

        return c;
    }

    CanonicalBorderInt operator>>(const CanonicalBorderInt& rhs) const {
        // ### rhs is actually a BaseType!!!!!
        CanonicalBorderInt c;
        CanonicalBaseType shift = rhs.content[CanonicalSize - 1];
        if (shift % BaseBitSize == 0) {
            // Whole shift:
            for (int i = CanonicalSize - 1; (i >= 0) && (signed(i - (shift / BaseBitSize)) >= 0); i--) {
                c.content[i] = content[i - (shift / BaseBitSize)];
            }
        } else {
            // [0001][1111][1110] >> 2
            // Partial shifts:
            CanonicalBaseType sright = shift % BaseBitSize;
            CanonicalBaseType sleft = BaseBitSize - sright;
            int idx, i;
            for (i = CanonicalSize - 1; i >= 0 && (signed(i - (shift / BaseBitSize)) >= 0); i--) {
                idx = i - (shift / BaseBitSize);
                if (idx - 1 >= 0)
                    c.content[i] = (content[idx] >> sright) | (content[idx-1] << sleft);
                else // Final round:
                    c.content[i] = (content[idx] >> sright);
            }
        }

        return c;
    }

    bool operator==(const CanonicalBorderInt& rhs) const {
        for (int i = 0; i < CanonicalSize; i++)
            if (content[i] != rhs.content[i])
                return false;
        return true;
    }

    bool operator!=(const CanonicalBorderInt& rhs) const {
        for (int i = 0; i < CanonicalSize; i++)
            if (content[i] == rhs.content[i])
                return false;
        return true;
    }

    bool operator<(const CanonicalBorderInt& rhs) const {
        for (int i = 0; i < CanonicalSize; i++)
            if (content[i] < rhs.content[i])
                return true;
        return false;
    }

    // Slow...
    CanonicalBorderInt& operator<<=(const CanonicalBorderInt& rhs) {
        *this = *this << rhs;
        return *this;
    }

    operator off_t() const {
        // Call only if offset fits in basetype!
        assert(sizeof(off_t) <= sizeof(CanonicalBaseType));
        // Only if this can be converted...
        // ### Check
        return content[CanonicalSize - 1];
    }
};


#endif // CANONICAL_BORDER_INT
