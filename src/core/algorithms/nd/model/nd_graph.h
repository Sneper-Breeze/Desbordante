#pragma once

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "algorithms/nd/model/nd_path.h"
#include "algorithms/nd/nd.h"
#include "model/table/column.h"
#include "model/table/vertical.h"

namespace model {

class NDGraph {
private:
    std::set<model::ND> full_arcs_;
    std::set<Vertical> nodes_;
    std::set<Column> simple_nodes_;
    std::map<Vertical, std::vector<model::ND>> full_arcs_map_;
    std::map<Vertical, std::vector<model::ND>> reverse_full_arcs_map_;
    std::multimap<Vertical, Column> dotted_arcs_;
    // It's sometimes convenient to count dotted arcs as FDs
    std::map<Vertical, std::vector<model::ND>> all_arcs_map_;
    std::map<Vertical, std::vector<model::ND>> reverse_all_arcs_map_;

    unsigned arity_{0};

    std::set<model::ND> AllExtensionsByTransitivity(unsigned max_arity) const;
    std::set<model::ND> AllExtensionsByUnion(unsigned max_arity) const;

public:
    /// @brief Create an ND-graph from a set of NDs
    NDGraph(std::set<model::ND> const& delta);

    /// @brief Create an ND-graph induced by a given node
    NDGraph(Vertical const& node);

    NDGraph(NDGraph const&) = default;
    NDGraph(NDGraph&&) = delete;
    NDGraph& operator=(NDGraph const&) = default;
    NDGraph& operator=(NDGraph&&) = delete;
    ~NDGraph() = default;

    std::set<Vertical> const& Nodes() const {
        return nodes_;
    }

    std::set<Column> const& Attr() const {
        return simple_nodes_;
    }

    std::vector<NDPath> SmartExtensions(NDPath const& g_pi);

    bool HasND(model::ND const& nd) const {
        return full_arcs_.find(nd) != full_arcs_.end();
    }

    std::set<ND> ReachableFrom(Vertical const& from) const;

    std::set<ND> ReverseReachableFrom(Vertical const& from) const;

    void Remove(ND const& nd);

    void RemoveUselessNDs(Vertical const& from, Vertical const& to);

    /// @brief Extend graph with the set of NDs
    /// @return @c true if any new ND was added
    bool Extend(std::set<ND> const& nds);

    /// @brief Extend graph with all NDs of arity <= max_arity
    void Closure(unsigned max_arity);

    // For debugging
    std::string ToNodesString() const {
        std::stringstream ss;
        ss << '{';
        for (auto pt{nodes_.begin()}; pt != nodes_.end(); ++pt) {
            if (pt != nodes_.begin()) {
                ss << ", ";
            }
            ss << pt->ToString();
        }
        ss << '}';
        return ss.str();
    }

    // For debugging
    std::string ToArcsWithPredicateString(std::function<bool(ND const&)> const& pred =
                                                  [](__attribute_maybe_unused__ ND const&) {
                                                      return true;
                                                  }) const {
        std::stringstream ss;
        ss << '{';
        for (auto const& arc : full_arcs_) {
            if (pred(arc)) {
                if (ss.peek() == '{') {
                    ss << ", ";
                }
                ss << arc;
            }
        }
        for (auto const& [start, end] : dotted_arcs_) {
            ND arc{start, Vertical{end}, 1};
            if (pred(arc)) {
                if (ss.peek() == '{') {
                    ss << ", ";
                }
                ss << "[dotted]" << arc;
            }
        }
        ss << '}';
        return ss.str();
    }

    // For debugging
    size_t NumNDs() const {
        return full_arcs_.size();
    }
};

}  // namespace model
