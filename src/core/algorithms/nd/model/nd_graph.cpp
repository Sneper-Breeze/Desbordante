#include "algorithms/nd/model/nd_graph.h"

#include <set>
#include <vector>

#include "algorithms/nd/nd.h"
#include "model/table/column.h"
#include "model/table/vertical.h"

namespace model {

std::vector<model::ND> NDGraph::AllExtensions(Vertical const& node) const {
    auto it = full_arcs_map_.find(node);
    if (it == full_arcs_map_.end()) {
        return std::vector<model::ND>();
    } else {
        return it->second;
    }
}

std::vector<model::ND> NDGraph::AllExtensions(Column const& node) const {
    Vertical vert_node{node};

    return AllExtensions(vert_node);
}

bool NDGraph::HasND(model::ND const& nd) const {
    auto const& lhs = nd.GetLhs();
    auto const& rhs = nd.GetRhs();
    for (auto const& candidate : full_arcs_) {
        if (candidate.GetLhs() == lhs && candidate.GetRhs() == rhs) {
            return true;
        }
    }
    return false;
}

}  // namespace model
