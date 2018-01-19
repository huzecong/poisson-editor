#include <functional>

#include "imagemagic.h"
#include "utils.h"

#include <QtCore>

#include <Eigen/SparseCore>
#include <Eigen/SparseCholesky>
#include <Eigen/IterativeLinearSolvers>

static const int dir[4][2] = {{0,  1},
                              {1,  0},
                              {0,  -1},
                              {-1, 0}};

typedef float Float;
typedef Eigen::VectorXf Vector;

struct Color {
    int col[3];

    Color() { memset(col, 0, sizeof col); }

    Color(int r, int g, int b) {
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
};

QImage ImageMagic::poissonFusion(const QImage &originalImage, const QImage &image, const QImage &mask) {
    int n = image.size().width(), m = image.size().height();
    auto isValid = [n, m](int x, int y) {
        return x >= 0 && x < n && y >= 0 && y < m;
    };

    qDebug() << "ImageMagic::poissonFusion perf";
    QElapsedTimer timer;
    timer.start();

    // Assign variables to interior pixels
    utils::Matrix<int> index(n, m);
    std::vector<QPoint> coordinates;
    /*
     * 0    -> exterior
     * >= 1 -> interior
     */
    int n_vars = 0;
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < m; ++j)
            if (mask.pixelColor(i, j).value() > 0) {
                index(i, j) = ++n_vars;
                coordinates.emplace_back(i, j);
            }
    qDebug() << "  1. mark pixels: " << timer.elapsed() << "ms";

    timer.restart();
    // Create coefficient matrix
    Eigen::SparseMatrix<Float> A(n_vars, n_vars);
    std::vector<Eigen::Triplet<Float>> coefficients;
    // |Np| ƒp  -  ∑{q ∈ Np ∩ Ω} ƒq  =  ∑{q ∈ Np ∩ ∂Ω} ƒ*q  +  ∑{q ∈ Np} v_pq
    for (int p = 0; p < n_vars; ++p) {
        int i = coordinates[p].x(), j = coordinates[p].y();

        int neighbors = 4;
        if (i == 0 || i == n - 1) --neighbors;
        if (j == 0 || j == m - 1) --neighbors;
        coefficients.emplace_back(p, p, static_cast<Float>(neighbors));

        for (int d = 0; d < 4; ++d) {
            int x = i + dir[d][0], y = j + dir[d][1];
            if (!isValid(x, y)) continue;
            int q = index(x, y) - 1;
            if (q >= 0) coefficients.emplace_back(p, q, -1.0);
        }
    }
    A.setFromTriplets(coefficients.begin(), coefficients.end());
    qDebug() << "  2. coef matrix: " << timer.elapsed() << "ms";

    Eigen::SimplicialLDLT<decltype(A)> solver(A);
//    Eigen::ConjugateGradient<decltype(A), Eigen::Upper | Eigen::Lower> solver(A);
    qDebug() << "  3. eigen compute: " << timer.elapsed() << "ms";

    QImage output = originalImage;

    // Create bias vector for each channel (RGB)
    std::vector<Vector> bs, xs;
    for (int ch = 0; ch < 3; ++ch)
        bs.emplace_back(n_vars);
    auto color = [](const QImage &img, int x, int y) {
        auto col = img.pixelColor(x, y);
        return Color(col.red(), col.green(), col.blue());
    };

    timer.restart();
    for (int p = 0; p < n_vars; ++p) {
        int i = coordinates[p].x(), j = coordinates[p].y();
        auto maskVal = mask.pixelColor(i, j).value();
        Color val;
        auto origColor = color(originalImage, i, j);
        auto patchColor = color(image, i, j);
        for (int d = 0; d < 4; ++d) {
            int x = i + dir[d][0], y = j + dir[d][1];
            if (!isValid(x, y)) continue;
            auto origNeighborColor = color(originalImage, x, y);
            auto patchNeighborColor = color(image, x, y);
//            if (mask.pixel(x, y) != maskVal) {
            if (mask.pixelColor(x, y).value() == 0) {
                // border pixel
                // val += origNeighborColor; + origColor - origNeighborColor;
                val += origColor;
//                val += patchNeighborColor + origColor - origNeighborColor;
            } else {
                if (mask.pixelColor(x, y).value() != maskVal) {
                    qDebug() << "ImageMagic::poissonfusion : Unmasked parts of patches overlap, falling back to naive copy-paste.";
                    return image;
                }
                for (int ch = 0; ch < 3; ++ch) {
                    float gradOrig = origColor.col[ch] - origNeighborColor.col[ch];
                    float gradPatch = patchColor.col[ch] - patchNeighborColor.col[ch];
                    val.col[ch] += std::abs(gradOrig) > std::abs(gradPatch) ? gradOrig : gradPatch;
                }
            }
        }
        bs[0][p] = val.col[0];
        bs[1][p] = val.col[1];
        bs[2][p] = val.col[2];
    }
    qDebug() << "  4. bias vectors: " << timer.elapsed() << "ms";

    timer.restart();
    for (int ch = 0; ch < 3; ++ch) {
        auto x = solver.solve(bs[ch]);
        xs.emplace_back(x);
    }
    qDebug() << "  5. eigen solve: " << timer.elapsed() << "ms";

    timer.restart();
    // Assemble solutions into output image
    for (int i = 0; i < n_vars; ++i) {
        int channels[3];
        for (int ch = 0; ch < 3; ++ch)
            channels[ch] = utils::clamp(static_cast<int>(round(xs[ch][i])), 0, 255);
        QColor col(channels[0], channels[1], channels[2]);
        output.setPixelColor(coordinates[i], col);
    }
    qDebug() << "  6. output: " << timer.elapsed() << "ms";

    return output;
}
