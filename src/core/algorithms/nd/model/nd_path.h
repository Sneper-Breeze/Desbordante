#pragma once

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "algorithms/nd/nd.h"
#include "model/table/column.h"
#include "model/table/vertical.h"

namespace model {

class NDPath {
private:
    std::set<ND> full_arcs_;
    std::set<Vertical> nodes_;
    std::set<Column> simple_nodes_;
    std::map<Vertical, std::vector<ND>> full_arcs_map_;
    std::multimap<Vertical, Column> dotted_arcs_;

    Vertical start_;
    std::shared_ptr<ND> last_added_;

public:
    /// @brief Create an ND-path from a set of NDs and with given start node
    NDPath(std::set<ND> const& delta, Vertical const& start,
           std::shared_ptr<ND> last_added = nullptr)
        : full_arcs_(delta), start_(start), last_added_(std::move(last_added)) {
        nodes_.insert(start_);

        // Fill nodes_ and full_arcs_map_:
        for (auto const& full_arc : full_arcs_) {
            auto const& lhs = full_arc.GetLhs();
            auto const& rhs = full_arc.GetRhs();

            nodes_.insert(lhs);
            nodes_.insert(rhs);

            if (full_arcs_map_.find(lhs) == full_arcs_map_.end()) {
                std::vector<ND> vec{full_arc};
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

    std::set<Vertical> const& Nodes() const {
        return nodes_;
    }

    std::set<Column> const& Attr() const {
        return simple_nodes_;
    }

    std::set<ND> const& NDs() const {
        return full_arcs_;
    }

    bool IsReachable(Column const& col) const {
        return simple_nodes_.find(col) != simple_nodes_.end();
    }

    bool HasND(ND const& nd) const {
        return full_arcs_.find(nd) != full_arcs_.end();
    }

    void Add(ND const& nd);

    NDPath Extend(ND const& nd) const;

    WeightType Weight() const;

    /// @brief Check if given ND can be added by rule 2 from ND-path definition
    bool CanAdd(ND const& nd) const;

    std::shared_ptr<ND> LastAdded() const {
        return last_added_;
    }

    bool IsDominatedBy(NDPath const& other) const;

    bool IsDominated(NDPath const& best, std::vector<NDPath>& active_paths) const;

    bool IsEssential(ND const& nd) const;

    /// @brief Checks if this ND-path is still an ND-path with the same start and Attr after ND
    /// removal
    bool CanSafelyRemove(ND const& nd) const;

    /// @brief Checks if this ND-path is still an ND-path with the same start after ND removal
    bool CanRemove(ND const& nd) const;

    bool IsMinimal() const;
};

}  // namespace model
