#include "algorithms/nd/util/set_operations.h"

namespace algos::nd::util {

template <Collection A, Set B>
bool IsSubsetOf(A const& a, B const& b) {
    for (auto const& elem : a) {
        if (b.find(elem) == b.end()) {
            return false;
        }
    }
    return true;
}

template <Collection A, Set B>
bool IsNotSubsetOf(A const& a, B const& b) {
    for (auto const& elem : a) {
        if (b.find(elem) == b.end()) {
            return true;
        }
    }
    return false;
}

template <InsertableCollection A, Set B>
A SetDifference(A const& a, B const& b) {
    A result;
    for (auto const& elem : a) {
        if (b.find(elem) == b.end()) {
            result.insert(elem);
        }
    }
    return result;
}

}  // namespace algos::nd::util