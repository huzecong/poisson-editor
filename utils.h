//
// Created by Kanari on 2018/1/9.
//

#ifndef POISSONEDITOR_UTILS_H
#define POISSONEDITOR_UTILS_H

namespace utils {
    template <typename T>
    T clamp(T x, T l, T r) {
        return x < l ? l : x > r ? r : x;
    }
}

#endif //POISSONEDITOR_UTILS_H
