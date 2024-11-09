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
    std::set<Column> simple_nodes_;

    Vertical start_;
    std::shared_ptr<ND> last_added_;

    bool IsDominatedBy(NDPath const& other) const;
    /// @brief Checks if this ND-path is still an ND-path with the same start and Attr after ND
    /// removal
    bool CanRemoveWithNoEffect(ND const& nd) const;

public:
    /// @brief Create an ND-path from a set of NDs and with given start node
    NDPath(std::set<ND> const& delta, Vertical const& start,
           std::shared_ptr<ND> last_added = nullptr);

    /// @brief Create an empty ND-path starting in @a start (\f$ G_\emptyset^X \f$)
    NDPath(Vertical const& start) : NDPath({}, start) {}

    NDPath(NDPath const&) = default;
    NDPath(NDPath&&) = default;
    NDPath& operator=(NDPath const&) = default;
    NDPath& operator=(NDPath&&) = default;
    ~NDPath() = default;

    std::set<Column> const& Attr() const {
        return simple_nodes_;
    }

    std::set<ND> const& NDs() const {
        return full_arcs_;
    }

    bool IsReachable(Column const& col) const {
        return simple_nodes_.contains(col);
    }

    bool HasND(ND const& nd) const {
        return full_arcs_.contains(nd);
    }

    void Add(ND const& nd);

    NDPath Extend(ND const& nd) const;

    WeightType Weight() const;

    /// @brief Check if given ND can be added by rule 2 from ND-path definition
    bool CanAdd(ND const& nd) const;

    std::shared_ptr<ND> LastAdded() const {
        return last_added_;
    }

    template <typename ActivePaths>
    bool IsDominated(NDPath const& best, ActivePaths& active_paths) const {
        if (IsDominatedBy(best)) {
            return true;
        }

        active_paths.EraseIf(
                [this](NDPath const& g_gamma) { return g_gamma.IsDominatedBy(*this); });
        return false;
    }

    bool IsEssential(ND const& nd) const;

    /// @brief Checks if this ND-path is still an ND-path with the same start after ND removal
    bool CanRemove(ND const& nd) const;

    bool IsMinimal() const;
};

}  // namespace model
