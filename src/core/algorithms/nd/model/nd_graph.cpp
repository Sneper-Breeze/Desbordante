#include "algorithms/nd/model/nd_graph.h"

#include <map>
#include <set>
#include <stack>
#include <vector>

#include "algorithms/nd/nd.h"
#include "algorithms/nd/util/set_operations.h"
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

std::set<ND> NDGraph::ReachableFrom(Vertical const& from) const {
    std::stack<Vertical> reachable;
    reachable.push(from);
    std::set<ND> visited;

    while (!reachable.empty()) {
        Vertical const& current = reachable.top();
        reachable.pop();
        if (full_arcs_map_.contains(current)) {
            for (ND const& nd : full_arcs_map_.at(current)) {
                visited.insert(nd);
                reachable.push(nd.GetRhs());
            }
        }
    }

    return visited;
}

std::set<ND> NDGraph::ReverseReachableFrom(Vertical const& from) const {
    std::stack<Vertical> reachable;
    for (Column const* col : from.GetColumns()) {
        for (Vertical const& vert : nodes_) {
            if (vert.Contains(*col)) {
                reachable.push(vert);
            }
        }
    }
    std::set<ND> visited;

    while (!reachable.empty()) {
        Vertical const& current = reachable.top();
        reachable.pop();
        if (reverse_full_arcs_map_.contains(current)) {
            for (ND const& nd : reverse_full_arcs_map_.at(current)) {
                visited.insert(nd);
                reachable.push(nd.GetLhs());
            }
        }
    }

    return visited;
}

void NDGraph::Remove(ND const& nd) {
    // Remove from full_arcs_:
    auto it = full_arcs_.find(nd);
    if (it == full_arcs_.end()) {
        return;
    }
    full_arcs_.erase(it);
    // Remove from full_arcs_map_:
    for (auto& pair : full_arcs_map_) {
        std::erase(pair.second, nd);
    }
    std::erase_if(full_arcs_map_, [](auto const& pair) { return pair.second.empty(); });
    // Remove from reverse_full_arcs_map_:
    for (auto& pair : reverse_full_arcs_map_) {
        std::erase(pair.second, nd);
    }
    std::erase_if(reverse_full_arcs_map_, [](auto const& pair) { return pair.second.empty(); });
    // Remove unreachable nodes and corresponding simple_nodes:
    auto const& lhs = nd.GetLhs();
    if (!full_arcs_map_.contains(lhs) && !reverse_full_arcs_map_.contains(lhs)) {
        // No other ND contains this node as lhs or rhs
        auto it = nodes_.find(lhs);
        if (it != nodes_.end()) {
            nodes_.erase(it);
            // Clear simple_nodes_:
            for (Column const* simple_node : lhs.GetColumns()) {
                bool found = false;
                for (Vertical const& node : nodes_) {
                    if (node.Contains(*simple_node)) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    // No other node contains this simple node
                    auto it = simple_nodes_.find(*simple_node);
                    if (it != simple_nodes_.end()) {
                        simple_nodes_.erase(it);
                    }
                }
            }
            // Clear full_arcs_map_ and reverse_full_arcs_map_:
            full_arcs_map_.erase(lhs);
            reverse_full_arcs_map_.erase(lhs);
        }
    }
    auto const& rhs = nd.GetRhs();
    if (!full_arcs_map_.contains(rhs) && !reverse_full_arcs_map_.contains(rhs)) {
        // No other ND contains this node as lhs or rhs
        auto it = nodes_.find(rhs);
        if (it != nodes_.end()) {
            nodes_.erase(it);
            for (Column const* simple_node : rhs.GetColumns()) {
                bool found = false;
                for (Vertical const& node : nodes_) {
                    if (node.Contains(*simple_node)) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    // No other node contains this simple node
                    auto it = simple_nodes_.find(*simple_node);
                    if (it != simple_nodes_.end()) {
                        simple_nodes_.erase(it);
                    }
                }
            }
            // Clear full_arcs_map_ and reverse_full_arcs_map_:
            full_arcs_map_.erase(rhs);
            reverse_full_arcs_map_.erase(rhs);
        }
    }
}

void NDGraph::RemoveUselessNDs(Vertical const& from, Vertical const& to) {
    // Forward search:
    std::set<ND> reachable = ReachableFrom(from);
    std::set<ND> diff = algos::nd::util::SetDifference(full_arcs_, reachable);
    for (auto const& nd : diff) {
        Remove(nd);
    }
    // Backward search:
    reachable = ReverseReachableFrom(to);
    diff = algos::nd::util::SetDifference(full_arcs_, reachable);
    for (auto const& nd : diff) {
        Remove(nd);
    }
}

}  // namespace model
