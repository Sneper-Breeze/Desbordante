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

/// @brief Checks if a is a subset of b
template <Collection A, Set B>
bool IsSubsetOf(A const& a, B const& b);

/// @brief Checks if a is not a subset of b
template <Collection A, Set B>
bool IsNotSubsetOf(A const& a, B const& b);

}  // namespace algos::nd::util