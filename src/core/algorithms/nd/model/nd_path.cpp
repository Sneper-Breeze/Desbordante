#include "algorithms/nd/model/nd_path.h"

#include <set>
#include <vector>

#include "algorithms/nd/model/nd_graph.h"
#include "algorithms/nd/nd.h"
#include "algorithms/nd/util/set_operations.h"
#include "model/table/column.h"
#include "model/table/vertical.h"

namespace model {

void NDPath::Add(ND const& nd) {
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
        full_arcs_map_.emplace(lhs, std::vector<ND>{nd});
    } else {
        full_arcs_map_[lhs].push_back(nd);
    }

    for (auto const& attrs : added_nodes) {
        if (attrs.GetArity() > 1) {
            for (Column const* attr : attrs.GetColumns()) {
                simple_nodes_.insert(*attr);
                nodes_.insert(Vertical(*attr));
                dotted_arcs_.emplace(attrs, *attr);
            }
        } else {
            simple_nodes_.insert(*(attrs.GetColumns().front()));
        }
    }
}

NDPath NDPath::Extend(ND const& nd) const {
    std::set<ND> new_delta{full_arcs_};
    new_delta.insert(nd);
    return NDPath(new_delta, start_);
}

WeightType NDPath::Weight() const {
    if (full_arcs_.empty()) {
        return 1;
    }

    WeightType result{1};
    for (auto const& nd : full_arcs_) {
        result *= nd.GetWeight();
    }
    return result;
}

bool NDPath::CanAdd(ND const& nd) const {
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

bool NDPath::IsDominatedBy(NDPath const& other) const {
    using namespace algos::nd::util;
    return IsSubsetOf(Attr(), other.Attr()) && other.Weight() <= Weight();
}

bool NDPath::IsDominated(NDPath const& best, std::vector<NDPath>& active_paths) const {
    if (IsDominatedBy(best)) {
        return true;
    }

    std::erase_if(active_paths,
                  [this](NDPath const& g_gamma) { return g_gamma.IsDominatedBy(*this); });
    return false;
}

bool NDPath::IsEssential(ND const& nd) const {
    // Check by lemma:
    std::set<ND> pi_i{full_arcs_};
    auto it = pi_i.find(nd);
    if (it == pi_i.end()) {
        return false;
    }
    pi_i.erase(it);
    NDGraph g_pi_i{pi_i};
    if (g_pi_i.Attr() == Attr()) {
        return false;
    }

    // Check by definition:
    for (Column const* col : nd.GetRhs().GetColumns()) {
        if (simple_nodes_.find(*col) == simple_nodes_.end()) {
            // Check that no other ND includes col in its Rhs:
            for (auto const& nd : full_arcs_) {
                if (nd.GetRhs().Contains(*col)) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool NDPath::CanSafelyRemove(ND const& nd) const {
    // Check if it's an ND-path with the same start:
    if (!CanRemove(nd)) {
        return false;
    }
    for (Column const* col : nd.GetRhs().GetColumns()) {
        // Some other ND must contain this attribute
        bool found = false;
        for (auto const& other : full_arcs_) {
            if (other != nd) {
                if (other.GetLhs().Contains(*col) || other.GetRhs().Contains(*col)) {
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

bool NDPath::CanRemove(ND const& nd) const {
    // It's enough to check if path remains connected
    if (start_ == nd.GetLhs()) {
        return false;
    }
    // Only NDs that are on the right end can be removed
    for (auto const& next : full_arcs_) {
        if (next.GetLhs() == nd.GetRhs()) {
            return false;
        }
    }
    return true;
}

bool NDPath::IsMinimal() const {
    for (auto const& nd : full_arcs_) {
        if (!IsEssential(nd) &&
            (last_added_ == nullptr || last_added_->GetRhs().Intersects(nd.GetRhs()))) {
            if (CanSafelyRemove(nd)) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace model
