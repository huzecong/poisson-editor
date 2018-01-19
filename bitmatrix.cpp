//
// Created by Kanari on 2018/1/19.
//

#include "bitmatrix.h"

BitMatrix::BitMatrix(const utils::Matrix<bool> &mat)
        : utils::Matrix<uchar>((mat.n + bitMask) >> logBits, mat.m), n_bits(mat.n), m_bits(mat.m) {
    int cnt = 0;
    for (int y = 0; y < m_bits; ++y) {
        for (int x = 0; x < n - 1; ++x) {
            uchar val = 0;
            int offset = x << logBits;
            for (int i = bitMask; i >= 0; --i)
                val = (val << 1) | static_cast<uchar>(mat(offset | i, y));
            arr[cnt++] = val;
        }
        {
            uchar val = 0;
            int offset = (n - 1) << logBits;
            for (int i = ((n_bits - 1) & bitMask); i >= 0; --i)
                val = (val << 1) | static_cast<uchar>(mat(offset | i, y));
            arr[cnt++] = val;
        }
    }
}

void BitMatrix::pasteSubMatrix(const BitMatrix &mat, int offsetX, int offsetY) {
    assert(mat.m_bits + offsetY <= m_bits && mat.n_bits + offsetX <= n_bits);
    assert(offsetX >= 0 && offsetY >= 0);
    int blocks = offsetX >> logBits, bits = offsetX & bitMask;
    if (bits == 0) {
        for (int y = 0; y < mat.m_bits; ++y) {
            int st = (y + offsetY) * n + blocks, matSt = y * mat.n;
            for (int x = 0; x < mat.n; ++x)
                arr[st + x] |= mat.arr[matSt + x];
        }
    } else {
        for (int y = 0; y < mat.m_bits; ++y) {
            int st = (y + offsetY) * n + blocks, matSt = y * mat.n;
            for (int x = 0; x < mat.n; ++x) {
                arr[st + x] |= mat.arr[matSt + x] << bits;
                arr[st + x + 1] |= mat.arr[matSt + x] >> (bitMask + 1 - bits);
            }
        }
    }
}
