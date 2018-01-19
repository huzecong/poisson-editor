#ifndef POISSONEDITOR_BITMATRIX_H
#define POISSONEDITOR_BITMATRIX_H

#include "utils.h"


// Transposed bit-compressed matrix
// Useful for converting into QBitmap, serving as masks for QPixmap
class BitMatrix : public utils::Matrix<uchar> {
    static const uchar bitMask = 7;
    static const int logBits = 3;

    class BitAccessor {
        BitMatrix *parent;
        int x, y;

        BitAccessor(BitMatrix *parent, int x, int y) : parent(parent), x(x), y(y) {}

        friend class BitMatrix;

    public:
        inline BitAccessor &operator =(bool val) {
            if (val) parent->set1(x, y);
            else parent->set0(x, y);
            return *this;
        }

        inline operator bool() const {
            return parent->get(x, y);
        }
    };

    int n_bits, m_bits;

    inline void set1(int x, int y) {
        arr[y * n + (x >> logBits)] |= 1 << (x & bitMask);
    }

    inline void set0(int x, int y) {
        arr[y * n + (x >> logBits)] &= ~(1 << (x & bitMask));
    }

    inline bool get(int x, int y) const {
        return static_cast<bool>(arr[y * n + (x >> logBits)] >> (x & bitMask) & 1);
    }

public:
    inline BitMatrix(int n, int m)
            : utils::Matrix<uchar>((n + bitMask) >> logBits, m), n_bits(n), m_bits(m) {}

    BitMatrix(const utils::Matrix<bool> &mat);

    inline BitAccessor operator ()(const QPoint &p) {
        return operator ()(p.x(), p.y());
    }

    inline BitAccessor operator ()(int x, int y) {
        assert(0 <= x && x < n_bits && 0 <= y && y < m_bits);
        return {this, x, y};
    }

    inline const uchar *toBytes() const {
        return arr;
    }

    void fill1();
    void invert();

    void subMatrixAnd(const BitMatrix &mat, int offsetX, int offsetY);
    void subMatrixOr(const BitMatrix &mat, int offsetX, int offsetY);
};


#endif //POISSONEDITOR_BITMATRIX_H
