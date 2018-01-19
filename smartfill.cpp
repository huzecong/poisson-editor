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
    int n, m, k;
    utils::Matrix<T> mat;

    virtual T update(const T &a, const T &b) const = 0;
    virtual T unitary() const = 0;

    inline bool isValid(int x, int y) {
        return x >= 0 && x < n && y >= 0 && y < m;
    }

    inline bool isValidWindow(int x, int y) {
        return x >= whl && x + whl < n && y >= whl && y + whl < m;
    }

public:
    SparseTable(int n, int m, int windowSize)
            : n(n), m(m), k(windowSize), mat(n, m) {}

    void modify(int x, int y, const T &val) {
        mat(x, y) = update(mat(x, y), val);
    }

    T query(int x, int y) {
        T ret = unitary();
        for (int dx = -whl; dx <= whl; ++dx)
            for (int dy = -whl; dy <= whl; ++dy)
                if (isValid(x + dx, y + dy))
                    ret = update(ret, mat(x + dx, y + dy));
        return ret;
    }
};

template <typename T>
class SumSparseTable : public SparseTable<T> {
    inline T update(const T &a, const T &b) const override {
        return a + b;
    }

    T unitary() const override {
        return 0;
    }

public:
    SumSparseTable(int n, int m, int k = windowSize) : SparseTable<T>(n, m, k) {}
};

template <typename T>
class MaxSparseTable : public SparseTable<T> {
    inline T update(const T &a, const T &b) const override {
        return std::max(a, b);
    }

    T unitary() const override {
        return 0;
    }

public:
    MaxSparseTable(int n, int m, int k = windowSize) : SparseTable<T>(n, m, k) {}
};

template<typename T>
class PairComparator {
public:
    inline bool operator ()(const T &a, const T &b) {
        return a.first < b.first;
    }
};

inline cv::Mat toCVMat(const utils::Matrix<int> &mat, int type = CV_32S) {
    cv::Mat ret(mat.rows(), mat.cols(), type);
    for (int i = 0; i < mat.rows(); ++i)
        for (int j = 0; j < mat.cols(); ++j)
            ret.at<int>(i, j) = mat(i, j);
    return ret;
}

template <typename T>
utils::Matrix<T> windowedSum(const utils::Matrix<T> &mat, int whl) {
    int n = mat.rows(), m = mat.cols();
    utils::Matrix<T> partial = mat;
    for (int i = 1; i < n; ++i)
        partial(i, 0) += partial(i - 1, 0);
    for (int j = 1; j < m; ++j)
        partial(0, j) += partial(0, j - 1);
    for (int i = 1; i < n; ++i)
        for (int j = 1; j < m; ++j)
            partial(i, j) += partial(i, j - 1) + partial(i - 1, j) - partial(i - 1, j - 1);

    auto getVal = [&](int x, int y) {
        if (x < 0 || y < 0) return 0;
        if (x >= n) x = n - 1;
        if (y >= m) y = m - 1;
        return partial(x, y);
    };
    utils::Matrix<T> result(n, m);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < m; ++j)
            result(i, j) = getVal(i + whl, j + whl) - getVal(i - whl, j + whl) - getVal(i + whl, j - whl) + getVal(i - whl, j - whl);
    return result;
}

using ImageMagic::Color;
using ImageMagic::dir;

class SmartFiller {
    static constexpr Float decay = 0.99;

    QImage image;
    BitMatrix mask;

    int n, m;

    SumSparseTable<float> confidenceTable;

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

    inline Float dataValue(int i, int j) {
        Float dataVal = 0.01;
        for (int d = 0; d < 4; ++d) {
            int x = i + dir[d][0], y = j + dir[d][1];
            if (isValid(x, y) && !mask(x, y))
                dataVal += colorDiff(x, y, i, j);
        }
        return std::abs(dataVal);
    };

    inline bool isFillFront(int i, int j) {
        if (mask(i, j) || !isValidWindow(i, j)) return false;
        for (int d = 0; d < 4; ++d) {
            int x = i + dir[d][0], y = j + dir[d][1];
            if (isValid(x, y) && mask(x, y)) return true;
        }
        return false;
    }

    int countMask(int i, int j) {
        int cnt = 0;
        for (int dx = -whl; dx <= whl; ++dx)
            for (int dy = -whl; dy <= whl; ++dy) {
                int x = i + dx, y = j + dy;
                if (isValid(x, y) && mask(x, y)) ++cnt;
            }
        return cnt;
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

        cv::Mat mat[3];
        for (int ch = 0; ch < 3; ++ch)
            mat[ch] = cv::Mat(n, m, CV_8UC1);

        while (true) {
            auto dataTable = MaxSparseTable<Float>(n, m);

            // Initialize data term values & find fill front pixels
            std::vector<QPoint> fillFront;
            for (int i = 0; i < n; ++i)
                for (int j = 0; j < m; ++j) {
                    if (!isFillFront(i, j)) continue;
                    Float dataVal = dataValue(i, j);
                    fillFront.emplace_back(i, j);
                    dataTable.modify(i, j, dataVal);
                }

            // Sort fill front pixels according to priority
            Float bestScore = INT_MIN;
            QPoint bestFront(-1, -1);
//        typedef std::pair<Float, QPoint> QueuePair;
//        std::priority_queue<QueuePair, std::vector<QueuePair>, PairComparator<QueuePair>> queue;
            for (auto &p : fillFront) {
                int x = p.x(), y = p.y();
                Float score = confidenceTable.query(x, y) * dataTable.query(x, y);// / countMask(x, y);
                if (score > bestScore) {
                    bestScore = score;
                    bestFront = p;
                }
            }

            if (bestFront.x() == -1) break;
            int x = bestFront.x(), y = bestFront.y();
            // Note that the priority score will never decrease
//            if (mask(x, y)) continue; // already filled

            // Initiliaze cv::Mat for convolution
            for (int i = 0; i < n; ++i)
                for (int j = 0; j < m; ++j)
                    if (mask(i, j)) {
                        auto col = Color(image.pixelColor(i, j));
                        for (int ch = 0; ch < 3; ++ch)
                            mat[ch].at<uchar>(i, j) = static_cast<uchar>(col.col[ch]);
//                    squared.at<int>(i, j) = sqr(col.col[0]) + sqr(col.col[1]) + sqr(col.col[2]);
                    } else {
                        for (int ch = 0; ch < 3; ++ch)
                            mat[ch].at<uchar>(i, j) = 0;
                    }

            cv::Mat squared(n, m, CV_32S);
            cv::Mat error(n, m, CV_32F);
            for (int i = 0; i < n; ++i)
                for (int j = 0; j < m; ++j)
                    squared.at<int>(i, j) = Color(image.pixelColor(i, j)).norm();
            cv::Mat maskKernel(windowSize, windowSize, CV_8UC1);
            for (int dx = -whl; dx <= whl; ++dx)
                for (int dy = -whl; dy <= whl; ++dy)
                    maskKernel.at<uchar>(dx + whl, dy + whl) = static_cast<uchar>(mask(x + dx, y + dy));
            cv::filter2D(squared, error, CV_32F, maskKernel);
//            squared = windowedSum(squared, whl);
//            cv::Mat error = toCVMat(squared);
            for (int ch = 0; ch < 3; ++ch) {
                cv::Mat result(n, m, CV_32F);
                cv::Mat kernel(windowSize, windowSize, CV_8UC1);
                for (int dx = -whl; dx <= whl; ++dx)
                    for (int dy = -whl; dy <= whl; ++dy)
                        kernel.at<uchar>(dx + whl, dy + whl) = static_cast<uchar>(Color(image.pixelColor(x + dx, y + dy)).col[ch]);
//                mat[ch](cv::Range(x - whl, x + whl), cv::Range(y - whl, y + whl)).copyTo(kernel);
                cv::filter2D(mat[ch], result, CV_32F, kernel);
                error -= result * 2;
            }

            BitMatrix validWindow = mask;
            for (auto &p : fillFront) {
                int x = p.x(), y = p.y();
                for (int dx = -whl; dx <= whl; ++dx)
                    for (int dy = -whl; dy <= whl; ++dy) {
                        int i = x + dx, j = y + dy;
                        if (isValid(i, j)) validWindow(i, j) = false;
                    }
            }

            float bestVal = INFI;
            QPoint bestPoint(-1, -1);
            for (int i = 0; i < n; ++i)
                for (int j = 0; j < m; ++j)
                    if (validWindow(i, j) && isValidWindow(i, j) && error.at<float>(i, j) < bestVal) {
                        bestVal = error.at<float>(i, j);
                        bestPoint = QPoint(i, j);
                    }
            int srcX = bestPoint.x(), srcY = bestPoint.y();
            qDebug() << bestFront << bestPoint;
//            for (int ch = 0; ch < 3; ++ch)
//                mat[ch](cv::Range(srcX - whl, srcX + whl), cv::Range(srcY - whl, srcY + whl))
//                        .copyTo(mat[ch](cv::Range(x - whl, x + whl), cv::Range(y - whl, y + whl)));

            float confidenceValue = confidenceTable.query(x, y) / (windowSize * windowSize);
            assert(confidenceValue < 1.0);
            for (int dx = -whl; dx <= whl; ++dx)
                for (int dy = -whl; dy <= whl; ++dy) {
                    int i = x + dx, j = y + dy;
                    if (!mask(i, j)) {
                        mask(i, j) = true;
                        auto col = Color(image.pixelColor(srcX + dx, srcY + dy));
                        image.setPixel(i, j, image.pixel(srcX + dx, srcY + dy));
                        confidenceTable.modify(i, j, confidenceValue);
                    }
                }
            break;
        }
        qDebug() << "done";

        return image;
    }
};

QImage ImageMagic::smartFill(const QImage &image, const BitMatrix &imageMask) {
    auto filler = SmartFiller(image, imageMask);
    return filler.compute();
}
