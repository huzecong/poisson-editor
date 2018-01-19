#ifndef POISSONEDITOR_UTILS_H
#define POISSONEDITOR_UTILS_H

#include <cassert>

#include <qmath.h>
#include <QRect>
#include <QRectF>


class BitMatrix;

namespace utils {
    template <typename T>
    T clamp(T x, T l, T r) {
        return x < l ? l : x > r ? r : x;
    }

    // Minimum containing rectangle with integer coordinates
    inline QRect toAlignedRect(const QRectF &rect) {
        QPoint topLeft = QPoint(qFloor(rect.x()), qFloor(rect.y()));
        QPoint bottomRight = QPoint(qCeil(rect.right()), qCeil(rect.bottom()));
        QRect alignedRect = QRect(topLeft.x(), topLeft.y(), bottomRight.x() - topLeft.x() + 1, bottomRight.y() - topLeft.y() + 1);
        return alignedRect;
    }

    template <typename T>
    class Matrix {
    protected:
        int n, m;
        T *arr;

        friend class ::BitMatrix;

    public:
        Matrix(int n, int m) : n(n), m(m) {
            arr = new T[n * m];
            memset(arr, 0, sizeof(T) * n * m);
        }

        inline const int rows() const {
            return n;
        }

        inline const int cols() const {
            return m;
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

}

#endif //POISSONEDITOR_UTILS_H
