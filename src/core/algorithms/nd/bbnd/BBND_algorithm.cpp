#include "algorithms/nd/bbnd/BBND_algorithm.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/unordered_map.hpp>
#include <easylogging++.h>

#include "algorithms/nd/util/active_nd_paths.h"
#include "algorithms/nd/util/build_initial_graph.h"
#include "config/descriptions.h"
#include "config/equal_nulls/option.h"
#include "config/indices/option.h"
#include "config/names.h"
#include "config/option.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"
#include "model/table/column_layout_relation_data.h"
#include "model/table/typed_column_data.h"
#include "model/types/builtin.h"
#include "model/types/type.h"

namespace algos {
Bbnd::Bbnd() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName(),
                          config::names::kMaximumLhs, config::names::kMaximumRhs});
}

void Bbnd::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
    RegisterOption(Option<size_t>(&max_lhs_arity_, kMaximumLhs, kDMaximumLhs, 2));
    RegisterOption(Option<size_t>(&max_rhs_arity_, kMaximumRhs, kDMaximumRhs, 2));
}

void Bbnd::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, is_null_equal_null_);
    input_table_->Reset();
    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: ND mining is meaningless.");
    }

    auto initial_nds = nd::util::BuildInitialGraph(*relation_);
    for (auto const& nd : initial_nds) {
        nd_collection_.Register(nd);
    }
    graph_ = std::make_shared<model::NDGraph>(initial_nds);
}

void Bbnd::MakeExecuteOptsAvailable() {
    // Add here any specific execution opts to be available in future
}

unsigned long long Bbnd::ExecuteInternal() {
    unsigned long long execution_time = 0;
    auto start = std::chrono::system_clock::now();

    auto const& all_columns = relation_->GetSchema()->GetColumns();

    auto bits_to_vertical = [&all_columns](std::vector<bool> const& bitset) -> Vertical {
        std::vector<Column> cols;
        for (size_t i{0}; i < bitset.size(); ++i) {
            if (bitset[i]) {
                cols.push_back(*all_columns[i]);
            }
        }
        return Vertical{cols.begin(), cols.end()};
    };

    // Here we need to enumerate all NDs with arity <= max_arity_ and derive 'em
    // FIXME(senichenkov): this loop is a nightmare
    for (size_t lhs_size{1}; lhs_size <= max_lhs_arity_; ++lhs_size) {
        for (size_t rhs_size{1}; rhs_size <= max_rhs_arity_; ++rhs_size) {
            if (lhs_size == rhs_size && rhs_size == 1) {  // 1-to-1 NDs are calculated directly
                continue;
            }
            LOG(INFO) << "Deriving " << lhs_size << "-to-" << rhs_size << " NDs";
            auto lhs_bit_subsets = SubsetsOfSize(all_columns.size(), lhs_size);
            auto rhs_bit_subsets = SubsetsOfSize(all_columns.size(), rhs_size);

            for (auto const& lhs_bitset : lhs_bit_subsets) {
                for (auto const& rhs_bitset : rhs_bit_subsets) {
                    auto lhs = bits_to_vertical(lhs_bitset);
                    auto rhs = bits_to_vertical(rhs_bitset);

                    auto nd = DeriveND(lhs, rhs);
                    nd_collection_.Register(nd);
                    graph_->Extend(std::set{nd});
                }
            }
        }
    }

    execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::system_clock::now() - start)
                             .count();
    return execution_time;
}

std::vector<std::vector<bool>> Bbnd::SubsetsOfSize(size_t superset_size, size_t subset_size) const {
    std::vector<std::vector<bool>> temp1{std::vector<bool>(superset_size)};
    std::vector<std::vector<bool>> temp2;
    std::vector<std::vector<bool>> result;

    auto len = [](std::vector<bool> const& vec) -> size_t {
        return std::count(vec.begin(), vec.end(), true);
    };

    for (size_t i{0}; i < superset_size; ++i) {
        temp2.clear();
        for (auto bitset : temp1) {
            auto l = len(bitset);
            if (l == subset_size) {
                result.push_back(std::move(bitset));
            } else if (l < subset_size) {
                // for each element either
                temp2.push_back(bitset);  // "disable" it
                bitset[i] = 1;
                ++l;
                if (l == subset_size) {
                    result.push_back(std::move(bitset));
                } else if (l < subset_size) {
                    temp2.push_back(std::move(bitset));  // or "enable" it
                }
            }
        }
        std::swap(temp1, temp2);
    }
    return result;
}

model::ND Bbnd::DeriveND(Vertical const& lhs, Vertical const& rhs) {
    LOG(INFO) << "Trying to derive " << lhs.ToString() << " -> " << rhs.ToString();
    model::WeightType weight{std::numeric_limits<model::WeightType>::max()};
    nd::util::ActiveNdPaths<nd::util::BeFComparator> activeNDPaths{rhs};

    graph_->Closure(std::max(lhs.GetArity(), rhs.GetArity()));

    std::set<Column> rhs_cols;
    for (auto* col : rhs.GetColumns()) {
        rhs_cols.insert(*col);
    }

    // So that NDs aren't being removed from original graph
    model::NDGraph graph_copy{*graph_};

    graph_copy.RemoveUselessNDs(lhs, rhs);

    activeNDPaths.Push({lhs});

    while (!activeNDPaths.IsEmpty()) {
        model::NDPath base_path = activeNDPaths.Pop();
        model::NDPath best_path{base_path};
        auto extensions = graph_copy.SmartExtensions(base_path);
        for (auto candidate : extensions) {
            auto w = candidate.Weight();
            if (std::includes(candidate.Attr().begin(), candidate.Attr().end(), rhs_cols.begin(),
                              rhs_cols.end())) {
                if (w < weight) {
                    weight = w;
                    best_path = candidate;
                }
            } else if (w < weight && candidate.IsDominated(best_path, activeNDPaths) &&
                       candidate.IsMinimal()) {
                activeNDPaths.Push(std::move(candidate));
            }
        }
    }

    return {lhs, rhs, weight};
}

}  // namespace algos
