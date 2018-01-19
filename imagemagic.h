#ifndef POISSONEDITOR_IMAGEMAGIC_H
#define POISSONEDITOR_IMAGEMAGIC_H

#include <QImage>

#include "bitmatrix.h"

namespace ImageMagic {

    QImage poissonFusion(const QImage &originalImage, const QImage &image, const QImage &mask);

    QImage smartFill(const QImage &image, const BitMatrix &mask);

}

#endif //POISSONEDITOR_IMAGEMAGIC_H
