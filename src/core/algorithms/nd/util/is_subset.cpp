#include "algorithms/nd/util/is_subset.h"

namespace algos::nd::util {

/// @brief Checks if a is a subset of b
template <Collection A, Set B>
bool IsSubsetOf(A const& a, B const& b) {
    for (auto const& elem : a) {
        if (b.find(elem) == b.end()) {
            return false;
        }
    }
    return true;
}

/// @brief Checks if a is not a subset of b
template <Collection A, Set B>
bool IsNotSubsetOf(A const& a, B const& b) {
    for (auto const& elem : a) {
        if (b.find(elem) == b.end()) {
            return true;
        }
    }
    return false;
}

}  // namespace algos::nd::util