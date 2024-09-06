#include <set>

#include <gtest/gtest.h>

#include "algorithms/nd/nd.h"
#include "algorithms/nd/util/build_initial_graph.h"
#include "algorithms/nd/util/set_operations.h"
#include "all_csv_configs.h"
#include "config/indices/type.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_index.h"
#include "model/table/column_layout_relation_data.h"

namespace tests {

config::InputTable CreateInputTable(CSVConfig const& csv_config) {
    return std::make_shared<CSVParser>(csv_config);
};

using NDTuple = std::tuple<std::vector<model::ColumnIndex>, std::vector<model::ColumnIndex>,
                           model::WeightType>;

NDTuple NDToTuple(model::ND const& nd) {
    return std::make_tuple(nd.GetLhsIndices(), nd.GetRhsIndices(), nd.GetWeight());
}

std::set<NDTuple> NDsToTuples(std::set<model::ND> const& nds) {
    std::set<NDTuple> result;
    for (model::ND const& nd : nds) {
        result.insert(NDToTuple(nd));
    }
    return result;
}

static auto const kTestNDInputTable = CreateInputTable(kTestND);

static std::set<NDTuple> const kTestNDNDs{{{0}, {1}, 4}, {{0}, {2}, 6}, {{0}, {3}, 4},
                                          {{0}, {4}, 5}, {{0}, {5}, 9}, {{0}, {6}, 3},
                                          {{1}, {0}, 1}, {{1}, {2}, 2}, {{1}, {3}, 2}};

struct BuildInitialGraphParams {
    config::InputTable input_table;
    bool null_eq_null;
    std::set<NDTuple> nds;
    bool extra_allowed;

    BuildInitialGraphParams(config::InputTable input_table, std::set<NDTuple> const& nds,
                            bool extra_allowed = false, bool null_eq_null = true)
        : input_table(std::move(input_table)),
          null_eq_null(null_eq_null),
          nds(nds),
          extra_allowed(extra_allowed) {}
};

class TestBuildInitialGraph : public ::testing::TestWithParam<BuildInitialGraphParams> {};

TEST_P(TestBuildInitialGraph, DefaultTest) {
    auto const& p = GetParam();
    auto input_table = p.input_table;
    auto null_eq_null = p.null_eq_null;
    auto nds = p.nds;
    auto extra_allowed = p.extra_allowed;

    auto relation = ColumnLayoutRelationData::CreateFrom(*input_table, null_eq_null);
    input_table->Reset();

    std::set<model::ND> actual_result = algos::nd::util::BuildInitialGraph(*relation);
    auto actual_result_tuples = NDsToTuples(actual_result);

    if (!extra_allowed) {
        EXPECT_EQ(actual_result_tuples, nds);
    } else {
        EXPECT_TRUE(algos::nd::util::IsSubsetOf(actual_result_tuples, nds));
    }
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    NDMiningTestsBuildInitialGraph, TestBuildInitialGraph,
    ::testing::Values(
        // Simple example from NDVerifier test suite:
        BuildInitialGraphParams(kTestNDInputTable, kTestNDNDs, true)
        ));
// clang-format on

}  // namespace tests
