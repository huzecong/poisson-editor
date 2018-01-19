//
// Created by Kanari on 2018/1/19.
//

#ifndef POISSONEDITOR_POISSONSOLVER_H
#define POISSONEDITOR_POISSONSOLVER_H

#include <QImage>

class PoissonSolver {
public:
    static QImage poissonFusion(const QImage &originalImage, const QImage &image, const QImage &mask);
};


#endif //POISSONEDITOR_POISSONSOLVER_H
