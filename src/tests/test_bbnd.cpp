#include <set>
#include <boost/dynamic_bitset.hpp>

#include <gtest/gtest.h>

#include "config/names.h"
#include "algorithms/algo_factory.h"
#include "algorithms/nd/nd.h"
#include "algorithms/nd/util/build_initial_graph.h"
#include "algorithms/nd/util/set_operations.h"
#include "all_csv_configs.h"
#include "config/indices/type.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_index.h"
#include "model/table/column_layout_relation_data.h"
#include "algorithms/nd/bbnd/BBND_algorithm.h"
#include "algorithms/nd/util/active_nd_paths.h"
#include "model/table/vertical.h"
#include "csv_config_util.h"

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

std::vector<NDTuple> NDsToTuples(std::vector<model::ND> const& nds) {
    std::vector<NDTuple> result;
    for (model::ND const& nd : nds) {
        result.push_back(NDToTuple(nd));
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

static std::vector<NDTuple> NDstoTest = {{{0, 1}, {3}, 8}, {{1}, {3, 0}, 9},
                                         {{4}, {5}, 3}, {{5}, {4}, 5}, 
                                         {{3, 6}, {2, 4}, 1}, {{2, 5}, {1, 3}, 5}};

Vertical CreateVertical(ColumnLayoutRelationData const& relation, std::vector<model::ColumnIndex> & indices) {
    boost::dynamic_bitset<> ind_bitset(relation.GetNumColumns());
    for(int i = 0; i < indices.size(); i++)
        ind_bitset.set(indices[i]);

    return Vertical(relation.GetSchema(), ind_bitset);
}

model::ND CreateNd(ColumnLayoutRelationData const& relation, NDTuple const& nd_to_create) {
    auto const& columns = relation.GetColumnData();
    boost::dynamic_bitset<> lhs_indices_(relation.GetNumColumns()), rhs_indices_(relation.GetNumColumns());
    auto [lhs, rhs, weight] = nd_to_create;

    return model::ND(CreateVertical(relation, lhs), CreateVertical(relation, rhs), weight);
}

struct ActiveNdPathsParams {
    config::InputTable input_table;
    std::vector<NDTuple> nds;
    boost::dynamic_bitset<> end_indices;
    bool null_eq_null;

    ActiveNdPathsParams(config::InputTable input_table, boost::dynamic_bitset<> end_indices,
                        std::vector<NDTuple> nds, bool null_eq_null = true)
        : end_indices(end_indices),
          nds(nds),
          input_table(std::move(input_table)),
          null_eq_null(null_eq_null) {}
};

class TestActiveNdPaths : public ::testing::TestWithParam<ActiveNdPathsParams> {};

TEST_P(TestActiveNdPaths, DefualtTest){
    auto const& p = GetParam();
    auto end_indices = p.end_indices;
    auto input_table = p.input_table;
    auto null_eq_null = p.null_eq_null;
    auto nds = p.nds;

    auto relation = ColumnLayoutRelationData::CreateFrom(*input_table, null_eq_null);
    input_table->Reset();

    Vertical end(relation->GetSchema(), end_indices);
    algos::nd::util::ActiveNdPaths<decltype(algos::nd::util::BeFCmpr)*> nd_queue(end);

    for(auto nd : nds){
        nd_queue.Push();
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

class TestBbndAlgorithm : public ::testing::Test{
protected:
    static std::unique_ptr<algos::Bbnd> CreateAndConfToLoad(CSVConfig const& csv_config) {
        using namespace config::names;
        using algos::ConfigureFromMap, algos::StdParamsMap;

        std::unique_ptr<algos::Bbnd> algorithm = std::make_unique<algos::Bbnd>();
        ConfigureFromMap(*algorithm, StdParamsMap{{kTable, MakeInputTable(csv_config)}});
        return algorithm;
    }

    static algos::StdParamsMap GetParamMap(
            CSVConfig const& csv_config,
            unsigned int max_lhs_ = std::numeric_limits<unsigned int>::max()) {
        using namespace config::names;
        // add more Params when algorithm will have it
        return {
                {kCsvConfig, csv_config},
                {kMaximumLhs, max_lhs_},
        };
    }
public:
    static std::unique_ptr<algos::Bbnd> CreateAlgorithmInstance(
            CSVConfig const& config,
            unsigned int max_lhs = std::numeric_limits<unsigned int>::max()) {
        return algos::CreateAndLoadAlgorithm<algos::Bbnd>(GetParamMap(config, max_lhs));
    }
};

TEST_F(TestBbndAlgorithm, InitTest) {
    ASSERT_THROW(CreateAlgorithmInstance(kTestEmpty);, std::runtime_error);
}
}  // namespace tests
