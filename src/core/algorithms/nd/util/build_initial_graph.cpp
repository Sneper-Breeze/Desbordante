#include "algorithms/nd/util/build_initial_graph.h"

#include <cassert>
#include <memory>
#include <set>
#include <unordered_map>

#include "algorithms/nd/nd.h"
#include "model/table/column_layout_relation_data.h"
#include "model/table/vertical.h"

namespace algos::nd::util {

std::set<model::ND> BuildInitialGraph(ColumnLayoutRelationData const& data) {
    std::set<model::ND> result;
    auto const& columns = data.GetColumnData();

    for (size_t i{0}; i < columns.size(); ++i) {
        for (size_t j{0}; j < columns.size(); ++j) {
            if (i == j) {
                // Loops aren't needed
                continue;
            }
            auto const& i_pt = columns[i].GetProbingTable();
            auto const& j_pt = columns[j].GetProbingTable();

            std::unordered_map<int, std::vector<int>> value_deps;
            for (size_t k{0}; k < i_pt.size(); ++k) {
                auto i_val = i_pt[k];
                auto j_val = j_pt[k];

                // Ignore unique values in lhs:
                if (i_val == 0) {
                    continue;
                }

                if (value_deps.contains(i_val)) {
                    if (j_val == 0) {
                        // These values are unique -- insert them all
                        value_deps[i_val].push_back(j_val);
                    } else {
                        // These values aren't unique -- check if same value already exists
                        bool found = false;
                        for (auto val : value_deps[i_val]) {
                            if (val == j_val) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            value_deps[i_val].push_back(j_val);
                        }
                    }
                } else {
                    value_deps.emplace(i_val, std::vector<int>({j_val}));
                }
            }

            model::WeightType weight = 0;
            for (auto [key, j_vals] : value_deps) {
                auto pair_weight = j_vals.size();
                if (pair_weight > weight) {
                    weight = pair_weight;
                }
            }

            Vertical lhs_vert{*(columns[i].GetColumn())};
            Vertical rhs_vert{*(columns[j].GetColumn())};
            result.emplace(lhs_vert, rhs_vert, weight);
        }
    }

    return result;
}

}  // namespace algos::nd::util
