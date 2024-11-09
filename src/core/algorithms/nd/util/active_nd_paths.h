#pragma once

#include <iostream>
#include <set>
#include <unordered_map>
#include <utility>

#include "algorithms/nd/model/nd_path.h"
#include "algorithms/nd/nd.h"
#include "algorithms/nd/util/active_nd_paths.h"
#include "model/table/column.h"
#include "model/table/column_layout_relation_data.h"
#include "model/table/vertical.h"

namespace algos::nd::util {
inline int IntersectionWithEnd(model::NDPath const& nd_path,
                               std::shared_ptr<std::set<Column>> const& end) {
    auto attrs = nd_path.Attr();

    int ans = 0;
    auto it_end = end->begin();
    for (auto it_attr = attrs.begin(); it_attr != attrs.end(); it_attr++) {
        while (it_end != end->end() && *it_end < *it_attr) it_end++;

        if (it_end != end->end() && *it_end == *it_attr) ans++;
    }

    return ans;
}

inline bool BeFCmpr(std::pair<model::NDPath, std::shared_ptr<std::set<Column>>> a,
                    std::pair<model::NDPath, std::shared_ptr<std::set<Column>>> b) {
    int res = IntersectionWithEnd(a.first, a.second) - IntersectionWithEnd(b.first, b.second);

    if (res > 0) {
        return true;
    }
    if (res == 0 && a.first.Weight() < b.first.Weight()) {
        return true;
    }

    return false;
}

struct BeFComparator {
    bool operator()(std::pair<model::NDPath, std::shared_ptr<std::set<Column>>> a,
                    std::pair<model::NDPath, std::shared_ptr<std::set<Column>>> b) const {
        return BeFCmpr(a, b);
    }
};

template <typename _Compare = std::less<model::NDPath>>
class ActiveNdPaths {
private:
    // int IntersectionWithEnd(model::NDPath nd_path);
    // bool CustomCmpr(model::NDPath a, model::NDPath b);

    std::set<std::pair<model::NDPath, std::shared_ptr<std::set<Column>>>, _Compare> queue_;
    std::shared_ptr<std::set<Column>> end_;

public:
    using It = std::set<std::pair<model::NDPath, std::shared_ptr<std::set<Column>>>,
                        _Compare>::iterator;
    using ElemT = std::pair<model::NDPath, std::shared_ptr<std::set<Column>>>;

    ActiveNdPaths(Vertical const& end) {
        end_ = std::make_shared<std::set<Column>>();
        for (Column const* col : end.GetColumns()) end_->insert(*col);

        queue_ = {};
    };

    ActiveNdPaths(std::set<Column>&& end) {
        end_ = std::make_shared<std::set<Column>>(std::move(end));
        queue_ = {};
    };

    // Changing methods
    model::NDPath Pop() {
        auto res = *queue_.begin();
        queue_.erase(queue_.begin());

        return res.first;
    };

    size_t EraseIf(std::function<bool(model::NDPath const&)> const& pred) {
        auto new_pred = [&pred](ElemT const& elem) -> bool { return pred(elem.first); };
        return std::erase_if(queue_, new_pred);
    }

    void Push(model::NDPath&& new_path) {
        queue_.emplace(new_path, end_);
    };

    // Checkout methods
    bool IsEmpty() const noexcept {
        return queue_.empty();
    };
};
}  // namespace algos::nd::util
