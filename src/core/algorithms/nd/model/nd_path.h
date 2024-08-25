#pragma once

#include <map>
#include <set>
#include <vector>

#include "algorithms/nd/nd.h"
#include "model/table/column.h"
#include "model/table/vertical.h"

namespace model {

class NDPath {
private:
    std::vector<model::ND> full_arcs_;
    std::set<Vertical> nodes_;
    std::set<Column> simple_nodes_;
    std::map<Vertical, std::vector<model::ND>> full_arcs_map_;
    std::multimap<Vertical, Column> dotted_arcs_;

    Vertical start_;

public:
    /// @brief Create an ND-path from a set of NDs and with given start node
    NDPath(std::vector<model::ND> const& delta, Vertical const& start)
        : full_arcs_(delta), start_(start) {
        nodes_.insert(start_);

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
                    dotted_arcs_.emplace(attrs, *attr);
                }
            } else {
                simple_nodes_.insert(*(attrs.GetColumns().front()));
            }
        }
    }

    std::set<Vertical> const& Nodes() const {
        return nodes_;
    }

    std::set<Column> const& Attr() const {
        return simple_nodes_;
    }

    bool IsReachable(Column const& col) const {
        return simple_nodes_.find(col) != simple_nodes_.end();
    }

    bool HasND(model::ND const& nd) const;

    void Add(model::ND const& nd);

    model::WeightType Weight() const;

    /// @brief Check if given ND can be added by rule 2 from ND-path definition
    bool CanAdd(model::ND const& nd) const;
};

}  // namespace model
