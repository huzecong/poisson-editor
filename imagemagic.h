#ifndef POISSONEDITOR_IMAGEMAGIC_H
#define POISSONEDITOR_IMAGEMAGIC_H

#include <QImage>
#include <QColor>

#include "bitmatrix.h"

namespace ImageMagic {

    static const int dir[4][2] = {{0,  1},
                                  {1,  0},
                                  {0,  -1},
                                  {-1, 0}};

    struct Color {
        int col[3];

        inline Color() { memset(col, 0, sizeof col); }

        inline Color(const QColor &color) {
            col[0] = color.red(), col[1] = color.green(), col[2] = color.blue();
        }

        inline Color(int r, int g, int b) {
            col[0] = r, col[1] = g, col[2] = b;
        }

        inline void operator +=(const Color &rhs) {
            col[0] += rhs.col[0];
            col[1] += rhs.col[1];
            col[2] += rhs.col[2];
        }

        inline Color operator +(const Color &rhs) {
            return {col[0] + rhs.col[0], col[1] + rhs.col[1], col[2] + rhs.col[2]};
        }

        inline Color operator -(const Color &rhs) {
            return {col[0] - rhs.col[0], col[1] - rhs.col[1], col[2] - rhs.col[2]};
        }

        inline int norm() const {
            return col[0] * col[0] + col[1] * col[1] + col[2] * col[2];
        }
    };

    QImage poissonFusion(const QImage &originalImage, const QImage &image, const QImage &mask);

    QImage smartFill(const QImage &image, const BitMatrix &mask);

}

#endif //POISSONEDITOR_IMAGEMAGIC_H
