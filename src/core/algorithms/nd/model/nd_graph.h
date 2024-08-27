#pragma once

#include <map>
#include <set>
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
    std::multimap<Vertical, Column> dotted_arcs_;

public:
    /// @brief Create an ND-graph from a set of NDs
    NDGraph(std::set<model::ND> const& delta) : full_arcs_(delta) {
        // Fill nodes_ and full_arcs_map_:
        for (auto const& full_arc : full_arcs_) {
            auto const& lhs = full_arc.GetLhs();
            auto const& rhs = full_arc.GetRhs();

            nodes_.insert(lhs);
            nodes_.insert(rhs);

            if (full_arcs_map_.find(lhs) == full_arcs_map_.end()) {
                std::vector<model::ND> vec{full_arc};
                full_arcs_map_.emplace(lhs, std::move(vec));
            } else {
                full_arcs_map_[lhs].push_back(full_arc);
            }
        }

        // Fill simple_nodes_:
        for (auto const& attrs : nodes_) {
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

    /// @brief Create an ND-graph induced by a given node
    NDGraph(Vertical const& node) {
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

    std::set<Vertical> const& Nodes() const {
        return nodes_;
    }

    std::set<Column> const& Attr() const {
        return simple_nodes_;
    }

    /// @brief All NDs that grow from the given node
    std::vector<model::ND> AllExtensions(Vertical const& node) const;

    /// @brief All NDs that grow from the given node
    std::vector<model::ND> AllExtensions(Column const& node) const;

    /// @brief All ND-paths obtained by extending G_pi with one full arc
    std::vector<NDPath> AllExtensions(NDPath const& g_pi) const;

    std::vector<NDPath> SmartExtensions(NDPath const& g_pi) const;

    bool HasND(model::ND const& nd) const {
        return full_arcs_.find(nd) != full_arcs_.end();
    }
};

}  // namespace model
