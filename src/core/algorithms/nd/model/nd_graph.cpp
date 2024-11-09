#include "algorithms/nd/model/nd_graph.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <stack>
#include <vector>

#include <easylogging++.h>

#include "algorithms/nd/nd.h"
#include "model/table/column.h"
#include "model/table/vertical.h"

namespace model {

std::set<ND> NDGraph::AllExtensionsByTransitivity(unsigned max_arity) const {
    std::set<ND> result;
    for (auto const& first : full_arcs_) {
        auto const& start = first.GetLhs();
        auto const& first_end = first.GetRhs();
        if (start.GetArity() > max_arity || first_end.GetArity() >= max_arity) {
            continue;
        }
        if (full_arcs_map_.contains(first_end)) {
            for (auto const& second : full_arcs_map_.at(first_end)) {
                auto const& second_end = second.GetRhs();
                if (first_end.GetArity() + second_end.GetArity() > max_arity) {
                    continue;
                }
                auto new_end = first_end.Union(second_end);
                auto new_weight = first.GetWeight() * second.GetWeight();
                result.emplace(start, new_end, new_weight);
            }
        }
    }
    return result;
}

std::set<ND> NDGraph::AllExtensionsByUnion(unsigned max_arity) const {
    std::set<ND> result;
    for (auto const& first : full_arcs_) {
        auto const& start = first.GetLhs();
        auto const& first_end = first.GetRhs();
        if (start.GetArity() > max_arity || first_end.GetArity() >= max_arity) {
            continue;
        }
        if (full_arcs_map_.contains(start)) {
            for (auto const& second : full_arcs_map_.at(start)) {
                auto const& second_end = second.GetRhs();
                if (first_end.GetArity() + second_end.GetArity() > max_arity) {
                    continue;
                }
                if (first_end != second_end) {
                    auto new_end = first_end.Union(second_end);
                    auto new_weight = first.GetWeight() * second.GetWeight();
                    result.emplace(start, new_end, new_weight);
                }
            }
        }
    }
    return result;
}

NDGraph::NDGraph(std::set<ND> const& delta) : full_arcs_(delta) {
    // Fill nodes_ and full_arcs_map_:
    for (auto const& full_arc : full_arcs_) {
        auto const& lhs = full_arc.GetLhs();
        auto const& rhs = full_arc.GetRhs();

        nodes_.insert(lhs);
        nodes_.insert(rhs);

        if (!full_arcs_map_.contains(lhs)) {
            std::vector<ND> vec{full_arc};
            full_arcs_map_.emplace(lhs, std::move(vec));
        } else {
            full_arcs_map_[lhs].push_back(full_arc);
        }

        if (!all_arcs_map_.contains(lhs)) {
            std::vector<ND> vec{full_arc};
            all_arcs_map_.emplace(lhs, std::move(vec));
        } else {
            all_arcs_map_[lhs].push_back(full_arc);
        }

        if (!reverse_full_arcs_map_.contains(rhs)) {
            std::vector<ND> vec{full_arc};
            reverse_full_arcs_map_.emplace(rhs, std::move(vec));
        } else {
            reverse_full_arcs_map_[rhs].push_back(full_arc);
        }

        if (!reverse_all_arcs_map_.contains(rhs)) {
            std::vector<ND> vec{full_arc};
            reverse_all_arcs_map_.emplace(rhs, std::move(vec));
        } else {
            reverse_all_arcs_map_[rhs].push_back(full_arc);
        }
    }

    // Fill simple_nodes_:
    for (auto const& attrs : nodes_) {
        if (attrs.GetArity() > 1) {
            for (Column const* attr : attrs.GetColumns()) {
                simple_nodes_.insert(*attr);
                nodes_.insert(Vertical(*attr));
                dotted_arcs_.emplace(attrs, *attr);

                if (all_arcs_map_.contains(attrs)) {
                    all_arcs_map_[attrs].emplace_back(attrs, Vertical(*attr), 1);
                } else {
                    std::vector<ND> vec{ND{attrs, Vertical(*attr), 1}};
                    all_arcs_map_.emplace(attrs, std::move(vec));
                }
            }
        } else {
            simple_nodes_.insert(*(attrs.GetColumns().front()));
        }
    }
}

NDGraph::NDGraph(Vertical const& node) {
    nodes_.insert(node);
    if (node.GetArity() > 1) {
        for (Column const* attr : node.GetColumns()) {
            simple_nodes_.insert(*attr);
            dotted_arcs_.emplace(node, *attr);
        }
    } else {
        simple_nodes_.insert(*(node.GetColumns().front()));
    }
}

std::vector<NDPath> NDGraph::SmartExtensions(NDPath const& g_pi) {
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
        Vertical current = reachable.top();
        reachable.pop();
        if (all_arcs_map_.contains(current)) {
            for (ND const& nd : all_arcs_map_.at(current)) {
                if (!visited.contains(nd)) {
                    visited.insert(nd);
                    reachable.push(nd.GetRhs());
                }
            }
        }
    }

    return visited;
}

std::set<ND> NDGraph::ReverseReachableFrom(Vertical const& from) const {
    std::stack<Vertical> reachable;
    reachable.push(from);
    std::set<ND> visited;

    while (!reachable.empty()) {
        Vertical current = reachable.top();
        reachable.pop();
        if (reverse_all_arcs_map_.contains(current)) {
            for (ND const& nd : reverse_all_arcs_map_.at(current)) {
                if (!visited.contains(nd)) {
                    visited.insert(nd);
                    reachable.push(nd.GetLhs());
                }
            }
        }
    }

    return visited;
}

void NDGraph::Remove(ND const& nd) {
    // Remove from full_arcs_:
    auto it = full_arcs_.find(nd);
    if (it != full_arcs_.end()) {
        full_arcs_.erase(it);
    }
    // Remove from full_arcs_map_:
    for (auto& pair : full_arcs_map_) {
        std::erase(pair.second, nd);
    }
    std::erase_if(full_arcs_map_, [](auto const& pair) { return pair.second.empty(); });
    // Remove from all_arcs_map_:
    for (auto& pair : all_arcs_map_) {
        std::erase(pair.second, nd);
    }
    std::erase_if(all_arcs_map_, [](auto const& pair) { return pair.second.empty(); });
    // Remove from reverse_full_arcs_map_:
    for (auto& pair : reverse_full_arcs_map_) {
        std::erase(pair.second, nd);
    }
    std::erase_if(reverse_full_arcs_map_, [](auto const& pair) { return pair.second.empty(); });
    // Remove from reverse_all_arcs_map_:
    for (auto& pair : reverse_all_arcs_map_) {
        std::erase(pair.second, nd);
    }
    std::erase_if(reverse_all_arcs_map_, [](auto const& pair) { return pair.second.empty(); });
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
            all_arcs_map_.erase(lhs);
            reverse_all_arcs_map_.erase(lhs);
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
            all_arcs_map_.erase(rhs);
            reverse_all_arcs_map_.erase(rhs);
        }
    }
}

void NDGraph::RemoveUselessNDs(Vertical const& from, Vertical const& to) {
    std::set<ND> reachable = ReachableFrom(from);
    std::set<ND> diff;
    std::set_difference(full_arcs_.begin(), full_arcs_.end(), reachable.begin(), reachable.end(),
                        std::inserter(diff, diff.end()));
    for (auto const& nd : diff) {
        Remove(nd);
    }
    // Backward search:
    reachable = ReverseReachableFrom(to);
    diff.clear();
    std::set_difference(full_arcs_.begin(), full_arcs_.end(), reachable.begin(), reachable.end(),
                        std::inserter(diff, diff.end()));
    for (auto const& nd : diff) {
        Remove(nd);
    }
}

bool NDGraph::Extend(std::set<ND> const& nds) {
    std::set<Vertical> new_nodes;
    bool any_new = false;

    // Fill full_arcs_, nodes_ and full_arcs_map_:
    for (auto const& full_arc : nds) {
        if (!full_arcs_.contains(full_arc)) {
            bool is_dominated = false;
            // FIXME(senichenkov): the whole method, and especially this check are terrible
            for (auto const& nd : full_arcs_) {
                if (nd.Dominates(full_arc)) {
                    is_dominated = true;
                    break;
                }
            }
            if (is_dominated) {
                continue;
            }
            any_new = true;

            full_arcs_.insert(full_arc);

            auto const& lhs = full_arc.GetLhs();
            auto const& rhs = full_arc.GetRhs();

            if (!nodes_.contains(lhs)) {
                nodes_.insert(lhs);
                new_nodes.insert(lhs);
            }
            if (!nodes_.contains(rhs)) {
                nodes_.insert(rhs);
                new_nodes.insert(rhs);
            }

            if (!full_arcs_map_.contains(lhs)) {
                std::vector<model::ND> vec{full_arc};
                full_arcs_map_.emplace(lhs, std::move(vec));
            } else {
                full_arcs_map_[lhs].push_back(full_arc);
            }

            if (!all_arcs_map_.contains(lhs)) {
                std::vector<ND> vec{full_arc};
                all_arcs_map_.emplace(lhs, std::move(vec));
            } else {
                all_arcs_map_[lhs].push_back(full_arc);
            }

            if (!reverse_full_arcs_map_.contains(rhs)) {
                std::vector<model::ND> vec{full_arc};
                reverse_full_arcs_map_.emplace(rhs, std::move(vec));
            } else {
                reverse_full_arcs_map_[rhs].push_back(full_arc);
            }

            if (!reverse_all_arcs_map_.contains(rhs)) {
                std::vector<model::ND> vec{full_arc};
                reverse_all_arcs_map_.emplace(rhs, std::move(vec));
            } else {
                reverse_all_arcs_map_[rhs].push_back(full_arc);
            }
        }
    }

    // Fill simple_nodes_:
    for (auto const& attrs : new_nodes) {
        if (attrs.GetArity() > 1) {
            for (Column const* attr : attrs.GetColumns()) {
                if (!simple_nodes_.contains(*attr)) {
                    simple_nodes_.insert(*attr);
                    nodes_.insert(Vertical(*attr));
                }
                dotted_arcs_.emplace(attrs, *attr);

                ND arc{attrs, Vertical(*attr), 1};
                if (all_arcs_map_.contains(attrs)) {
                    all_arcs_map_[attrs].push_back(arc);
                } else {
                    std::vector<ND> vec{arc};
                    all_arcs_map_.emplace(attrs, std::move(vec));
                }
            }
        } else {
            simple_nodes_.insert(*(attrs.GetColumns().front()));
        }
    }
    return any_new;
}

void NDGraph::Closure(unsigned max_arity) {
    if (arity_ >= max_arity) {
        return;
    }
    while (true) {
        auto trans_closure = AllExtensionsByTransitivity(max_arity);
        auto union_closure = AllExtensionsByUnion(max_arity);

        trans_closure.insert(union_closure.begin(), union_closure.end());

        if (!Extend(trans_closure)) {
            break;
        }
    }
    arity_ = max_arity;
}

}  // namespace model
