#include "algorithms/nd/util/active_nd_paths.h"
#include "model/table/column.h"
#include "model/table/vertical.h"
#include <set>


namespace algos::nd::util {
template<typename _Compare>
ActiveNdPaths<_Compare>::ActiveNdPaths(Vertical end) {
    std::vector<Column const*> end_columns = end.GetColumns();
    end_ = std::set<Column>(end_columns.begin(), end_columns.end());
    queue_ = {};
}

template<typename _Compare>
ActiveNdPaths<_Compare>::ActiveNdPaths(std::set<Column> && end) {
    end_ = std::move(end);
    queue_ = {};
}

template<typename _Compare>
model::NDPath ActiveNdPaths<_Compare>::Pop() {
    auto res = *queue_.begin();
    queue_.erase(queue_.begin());

    return res.first;
}

template<typename _Compare>
void ActiveNdPaths<_Compare>::Push(model::NDPath && new_path){
    queue_.emplace({new_path, end_});
}

} // namespace algos::nd::util