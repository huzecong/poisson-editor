#include <queue>

#include "imagemagic.h"
#include "utils.h"

#include <QPoint>
#include "qdebug.h"

#include <opencv2/opencv.hpp>

static const int whl = 4; // window half length
static const int windowSize = whl * 2 + 1;

static const float INFI = (float)windowSize * windowSize * 255 * 255;

typedef float Float;

template <typename T>
class SparseTable {
    int n, m;
    utils::Matrix<T> mat;

    inline bool isValid(int x, int y) {
        return x >= 0 && x < n && y >= 0 && y < m;
    }

public:
    SparseTable(int n, int m)
            : n(n), m(m), mat(n, m) {}

    void modify(int x, int y, const T &val) {
        mat(x, y) = val;
    }

    T query(int x, int y) {
        T ret = 0;
        for (int dx = -whl; dx <= whl; ++dx)
            for (int dy = -whl; dy <= whl; ++dy)
                ret += mat(x + dx, y + dy);
        return ret;
    }
};

using ImageMagic::Color;
using ImageMagic::dir;

class SmartFiller {
    QImage image;
    BitMatrix mask;

    int n, m;

    SparseTable<float> confidenceTable;

    inline bool isValid(int x, int y) {
        return x >= 0 && x < n && y >= 0 && y < m;
    }

    inline bool isValidWindow(int x, int y) {
        return x >= whl && x + whl < n && y >= whl && y + whl < m;
    }

    inline int colorDiff(int x1, int y1, int x2, int y2) {
        auto col1 = image.pixelColor(x1, y1), col2 = image.pixelColor(x2, y2);
        int val = (col1.red() - col2.red()) + (col1.green() - col2.green()) + (col1.blue() - col2.blue());
        return val;
    };

    inline bool isFillFront(int i, int j) {
        if (mask(i, j) || !isValidWindow(i, j)) return false;
        for (int d = 0; d < 4; ++d) {
            int x = i + dir[d][0], y = j + dir[d][1];
            if (isValid(x, y) && mask(x, y)) return true;
        }
        return false;
    }

public:
    SmartFiller(const QImage &image, const BitMatrix &mask)
            : image(image), mask(mask), n(image.width()), m(image.height()),
              confidenceTable(n, m) {}

    QImage compute() {
        // Initialize confidence term values
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < m; ++j)
                if (mask(i, j)) confidenceTable.modify(i, j, 1.0);

        int totalPixels = 0;
        // Initiliaze cv::Mat for convolution
        cv::Mat mat[3];
        cv::Mat squared(n, m, CV_32S);
        for (int ch = 0; ch < 3; ++ch)
            mat[ch] = cv::Mat(n, m, CV_8UC1);
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < m; ++j)
                if (mask(i, j)) {
                    auto col = Color(image.pixelColor(i, j));
                    for (int ch = 0; ch < 3; ++ch)
                        mat[ch].at<uchar>(i, j) = static_cast<uchar>(col.col[ch]);
                    squared.at<int>(i, j) = col.norm();
                } else {
                    ++totalPixels;
                    for (int ch = 0; ch < 3; ++ch)
                        mat[ch].at<uchar>(i, j) = 0;
                }

        int progress = 0;
        while (true) {
            // Initialize data term values & find fill front pixels
            // Sort fill front pixels according to priority
            Float bestScore = INT_MIN;
            QPoint bestTgt(-1, -1);
            std::vector<QPoint> fillFront;
            for (int i = 0; i < n; ++i)
                for (int j = 0; j < m; ++j) {
                    if (!isFillFront(i, j)) continue;
                    fillFront.emplace_back(i, j);
                    int nX = mask(i + 1, j) - mask(i - 1, j);
                    int nY = mask(i, j + 1) - mask(i, j - 1);
                    Float dataVal = 0.0;
                    if (nX != 0 || nY != 0) {
                        int maxVal = 0;
                        for (int dx = -whl; dx <= whl; ++dx)
                            for (int dy = -whl; dy <= whl; ++dy) {
                                int x = i + dx, y = j + dy;
                                if (!(mask(x + 1, y) && mask(x - 1, y) && mask(x, y + 1) && mask(x, y - 1))) continue;
                                int dX = colorDiff(x + 1, y, x - 1, y);
                                int dY = colorDiff(x, y + 1, x, y - 1);
                                maxVal = std::max(maxVal, std::abs(dX * nX + dY * nY));
                            }
                        auto len = static_cast<float>(sqrt(nX * nX + nY * nY));
                        dataVal = maxVal / len;
                    }
                    Float score = confidenceTable.query(i, j) * (dataVal + 0.001f);
                    if (score > bestScore) {
                        bestScore = score;
                        bestTgt = {i, j};
                    }
                }
            if (bestTgt.x() == -1) break;
            int x = bestTgt.x(), y = bestTgt.y();

            // Calculate MSE between kernel and all patches
            cv::Mat error(n, m, CV_32S);
            cv::Mat maskKernel(windowSize, windowSize, CV_8UC1);
            for (int dx = -whl; dx <= whl; ++dx)
                for (int dy = -whl; dy <= whl; ++dy)
                    maskKernel.at<uchar>(dx + whl, dy + whl) = static_cast<uchar>(mask(x + dx, y + dy));
            cv::filter2D(squared, error, CV_32S, maskKernel);
            for (int ch = 0; ch < 3; ++ch) {
                cv::Mat result(n, m, CV_32S);
                cv::Mat kernel(windowSize, windowSize, CV_8UC1);
                for (int dx = -whl; dx <= whl; ++dx)
                    for (int dy = -whl; dy <= whl; ++dy)
                        kernel.at<uchar>(dx + whl, dy + whl) = mat[ch].at<uchar>(x + dx, y + dy);
                cv::filter2D(mat[ch], result, CV_32S, kernel);
                error -= result * 2;
            }

            // Filter out partially filled patches
            BitMatrix validWindow = mask;
            for (auto &p : fillFront) {
                int x = p.x(), y = p.y();
                for (int dx = -whl; dx <= whl; ++dx)
                    for (int dy = -whl; dy <= whl; ++dy) {
                        int i = x + dx, j = y + dy;
                        if (isValid(i, j)) validWindow(i, j) = false;
                    }
            }

            // Find the best fit patch
            float bestVal = INFI;
            QPoint bestSrc(-1, -1);
            for (int i = 0; i < n; ++i)
                for (int j = 0; j < m; ++j)
                    if (validWindow(i, j) && isValidWindow(i, j)) {
                        float val = error.at<int>(i, j);
                        if (val < bestVal) {
                            bestVal = val;
                            bestSrc = QPoint(i, j);
                        }
                    }
            int srcX = bestSrc.x(), srcY = bestSrc.y();
//            qDebug() << bestTgt << bestSrc;

            // Modify existing matrices
            float confidenceValue = confidenceTable.query(x, y) / (windowSize * windowSize);
            assert(confidenceValue < 1.0);
            for (int dx = -whl; dx <= whl; ++dx)
                for (int dy = -whl; dy <= whl; ++dy) {
                    int i = x + dx, j = y + dy;
                    if (!mask(i, j)) {
                        ++progress;
                        mask(i, j) = true;
                        auto col = Color(image.pixelColor(srcX + dx, srcY + dy));
                        for (int ch = 0; ch < 3; ++ch)
                            mat[ch].at<uchar>(i, j) = static_cast<uchar>(col.col[ch]);
                        squared.at<int>(i, j) = col.norm();
                        image.setPixel(i, j, image.pixel(srcX + dx, srcY + dy));
                        confidenceTable.modify(i, j, confidenceValue);
                    }
                }
            qDebug() << progress << "/" << totalPixels;

            if (progress > 500) break;
        }
        qDebug() << "done";

        return image;
    }
};

QImage ImageMagic::smartFill(const QImage &image, const BitMatrix &imageMask) {
    auto filler = SmartFiller(image, imageMask);
    return filler.compute();
}
