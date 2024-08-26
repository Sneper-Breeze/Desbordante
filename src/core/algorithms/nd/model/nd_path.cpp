#include "algorithms/nd/model/nd_path.h"

#include <set>
#include <vector>

#include "algorithms/nd/nd.h"
#include "model/table/column.h"
#include "model/table/vertical.h"

namespace model {

void NDPath::Add(model::ND const& nd) {
    if (HasND(nd)) {
        return;
    }

    full_arcs_.insert(nd);

    auto lhs = nd.GetLhs();
    auto rhs = nd.GetRhs();
    std::set<Vertical> added_nodes;

    nodes_.insert(lhs);
    added_nodes.insert(lhs);
    nodes_.insert(rhs);
    added_nodes.insert(rhs);

    if (full_arcs_map_.find(lhs) == full_arcs_map_.end()) {
        full_arcs_map_.emplace(lhs, std::vector<model::ND>{nd});
    } else {
        full_arcs_map_[lhs].push_back(nd);
    }

    for (auto const& attrs : added_nodes) {
        if (attrs.GetArity() > 1) {
            for (Column const* attr : attrs.GetColumns()) {
                simple_nodes_.insert(*attr);
                dotted_arcs_.emplace(attrs, *attr);
            }
        } else {
            simple_nodes_.insert(*(attrs.GetColumns().front()));
        }
    }
}

NDPath NDPath::Extend(model::ND const& nd) const {
    std::set<model::ND> new_delta{full_arcs_};
    new_delta.insert(nd);
    return NDPath(new_delta, start_);
}

model::WeightType NDPath::Weight() const {
    if (full_arcs_.empty()) {
        return 1;
    }

    model::WeightType result{1};
    for (auto const& nd : full_arcs_) {
        result *= nd.GetWeight();
    }
    return result;
}

bool NDPath::CanAdd(model::ND const& nd) const {
    for (Column const* col : nd.GetLhs().GetColumns()) {
        if (simple_nodes_.find(*col) == simple_nodes_.end()) {
            return false;
        }
    }

    for (Column const* col : nd.GetRhs().GetColumns()) {
        if (simple_nodes_.find(*col) == simple_nodes_.end()) {
            return true;
        }
    }
    return false;
}

}  // namespace model
