#pragma once

namespace algos::nd::util {

template <typename T>
concept Set = requires(T const& t) {
    t.find(*(t.begin())) == t.end();
};

template <typename T>
concept Collection = requires(T const& t) {
    t.begin();
    t.end();
};

template <typename T>
concept InsertableCollection = requires(T& t, T::value_type const& elem) {
    t.end();
    t.begin();
    t.insert(elem);
};

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

/// @brief Calculates a \ b
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