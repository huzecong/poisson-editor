//
// Created by Kanari on 2018/1/9.
//

#ifndef POISSONEDITOR_UTILS_H
#define POISSONEDITOR_UTILS_H

#include <cassert>

namespace utils {
    template <typename T>
    T clamp(T x, T l, T r) {
        return x < l ? l : x > r ? r : x;
    }

    template <typename T>
    class Matrix {
    protected:
        int n, m;
        T *arr;

    public:
        Matrix(int n, int m) : n(n), m(m) {
            arr = new T[n * m];
            memset(arr, 0, sizeof(T) * n * m);
        }

        Matrix(const Matrix &mat) {
            n = mat.n, m = mat.m;
            arr = new T[n * m];
            memcpy(arr, mat.arr, sizeof(T) * n * m);
        }

        Matrix(Matrix &&mat) noexcept {
            n = mat.n, m = mat.m;
            arr = mat.arr;
            mat.arr = nullptr;
        }

        Matrix &operator =(const Matrix &mat) {
            n = mat.n, m = mat.m;
            arr = new T[n * m];
            memcpy(arr, mat.arr, sizeof(T) * n * m);
            return *this;
        }

        Matrix &operator =(Matrix &&mat) noexcept {
            n = mat.n, m = mat.m;
            arr = mat.arr;
            mat.arr = nullptr;
            return *this;
        }

        ~Matrix() {
            delete[] arr;
        }

        inline T &operator ()(const QPoint &p) {
            return operator ()(p.x(), p.y());
        }

        inline const T &operator ()(const QPoint &p) const {
            return operator ()(p.x(), p.y());
        }

        inline T &operator ()(int x, int y) {
            assert(0 <= x && x < n && 0 <= y && y < m);
            return arr[x * m + y];
        }

        inline const T &operator ()(int x, int y) const {
            assert(0 <= x && x < n && 0 <= y && y < m);
            return arr[x * m + y];
        }
    };

    class BitMatrix : Matrix<uchar> {
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

        void set1(int x, int y) {
            arr[y * n + (x >> 3)] |= 1 << (x & 7);
        }

        void set0(int x, int y) {
            arr[y * n + (x >> 3)] &= ~(1 << (x & 7));
        }

        bool get(int x, int y) const {
            return static_cast<bool>(arr[y * n + (x >> 3)] >> (x & 7) & 1);
        }

    public:
        BitMatrix(int n, int m) : Matrix<uchar>((n + 7) >> 3, m), n_bits(n), m_bits(m) {}

        inline BitAccessor operator ()(const QPoint &p) {
            return operator ()(p.x(), p.y());
        }

        inline BitAccessor operator ()(int x, int y) {
            assert(0 <= x && x < n_bits && 0 <= y && y < m_bits);
            return BitAccessor(this, x, y);
        }

        const uchar *toBytes() const {
            return arr;
        }
    };
}

#endif //POISSONEDITOR_UTILS_H
