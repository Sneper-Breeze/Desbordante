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

std::vector<NDPath> NDGraph::AllExtensions(NDPath const& g_pi) const {
    std::vector<NDPath> result;
    for (auto const& nd : full_arcs_) {
        if (!(g_pi.HasND(nd))) {
            if (g_pi.CanAdd(nd)) {
                result.push_back(g_pi.Extend(nd));
            }
        }
    }
    return result;
}

std::vector<NDPath> NDGraph::SmartExtensions(NDPath const& g_pi) const {
    std::vector<NDPath> result;
    auto last_added = g_pi.LastAdded();
    for (auto const& nd : full_arcs_) {
        if (!(g_pi.HasND(nd))) {
            if (last_added == nullptr || nd < *last_added) {
                if (g_pi.CanAdd(nd)) {
                    result.push_back(g_pi.Extend(nd));
                }
            }
        }
    }
    return result;
}

}  // namespace model
