#include "algorithms/nd/model/nd_path.h"

#include <numeric>
#include <set>
#include <vector>

#include <easylogging++.h>

#include "algorithms/nd/model/nd_graph.h"
#include "algorithms/nd/nd.h"
#include "model/table/column.h"
#include "model/table/vertical.h"

namespace model {

NDPath::NDPath(std::set<ND> const& delta, Vertical const& start, std::shared_ptr<ND> last_added)
    : full_arcs_(delta), start_(start), last_added_(std::move(last_added)) {
    auto decompose = [this](Vertical const& attrs) {
        auto const& cols = attrs.GetColumns();
        std::transform(cols.begin(), cols.end(), std::inserter(simple_nodes_, simple_nodes_.end()),
                       [](Column const* col) { return *col; });
    };

    decompose(start);

    for (auto const& full_arc : full_arcs_) {
        decompose(full_arc.GetLhs());
        decompose(full_arc.GetRhs());
    }
}

void NDPath::Add(ND const& nd) {
    if (HasND(nd)) {
        return;
    }

    full_arcs_.insert(nd);

    auto decompose = [this](Vertical const& attrs) {
        auto const& cols = attrs.GetColumns();
        std::transform(cols.begin(), cols.end(), std::inserter(simple_nodes_, simple_nodes_.end()),
                       [](Column const* col) { return *col; });
    };

    decompose(nd.GetLhs());
    decompose(nd.GetRhs());

    last_added_ = std::make_shared<ND>(nd);
}

NDPath NDPath::Extend(ND const& nd) const {
    std::set<ND> new_delta{full_arcs_};
    new_delta.insert(nd);
    return NDPath(new_delta, start_);
}

WeightType NDPath::Weight() const {
    WeightType result{1};
    for (auto const& nd : full_arcs_) {
        result *= nd.GetWeight();
    }
    return result;
}

bool NDPath::CanAdd(ND const& nd) const {
    // ND van be added if Lhs is subset of Attr and Rhs is not subset of Attr
    // (i. e. path must "grow")
    for (Column const* col : nd.GetLhs().GetColumns()) {
        if (!simple_nodes_.contains(*col)) {
            return false;
        }
    }

    for (Column const* col : nd.GetRhs().GetColumns()) {
        if (!simple_nodes_.contains(*col)) {
            return true;
        }
    }
    return false;
}

bool NDPath::IsDominatedBy(NDPath const& other) const {
    return (std::includes(other.Attr().begin(), other.Attr().end(), Attr().begin(),
                          Attr().end())) &&
           other.Weight() <= Weight();
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

bool NDPath::CanRemoveWithNoEffect(ND const& nd) const {
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
            if (CanRemoveWithNoEffect(nd)) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace model
