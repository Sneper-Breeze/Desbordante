#include <set>

#include <boost/dynamic_bitset.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "algorithms/algo_factory.h"
#include "algorithms/nd/bbnd/BBND_algorithm.h"
#include "algorithms/nd/nd.h"
#include "algorithms/nd/util/active_nd_paths.h"
#include "algorithms/nd/util/build_initial_graph.h"
#include "algorithms/nd/util/set_operations.h"
#include "all_csv_configs.h"
#include "config/indices/type.h"
#include "config/names.h"
#include "config/tabular_data/input_table_type.h"
#include "csv_config_util.h"
#include "model/table/column_index.h"
#include "model/table/column_layout_relation_data.h"
#include "model/table/vertical.h"

namespace tests {

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

/* For future debaging
void PrintNd(NDTuple const& nd) {
    std::cout << "LHS:" << std::endl;
    for(auto indice : std::get<0>(nd))
        std::cout << indice << std::endl;

    std::cout << "RHS:" << std::endl;
    for(auto indice : std::get<1>(nd))
        std::cout << indice << std::endl;

    std::cout << "Weirgt:" << std::endl;
    std::cout << std::get<2>(nd) << std::endl;;
}

void PrintNds(std::set<NDTuple> const& nds) {
    for (auto const& nd : nds) {
        std::cout << "ND:";
        PrintNd(nd);
    }
    std::cout << "\n\n";
}
*/

static auto const kTestNDInputTable = MakeInputTable(kTestND);

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
        EXPECT_THAT(actual_result_tuples, ::testing::IsSupersetOf(nds));
    }
}

static std::vector<std::set<NDTuple>> kTestNd_Paths = {
        {{{0, 1}, {3}, 8}, {{1}, {4, 0}, 9}},  // inters: 2, w:72
        {{{1}, {5, 0}, 3}},
        {{{0, 1}, {5}, 5}},                             // inters: 1 w: 3 5
        {{{1, 6}, {5, 4}, 10}, {{5, 1}, {3, 2}, 10}}};  // inters 3 w: 100

class ActiveNdPathsDataFrame {
private:
    RelationalSchema const* relation;

public:
    ActiveNdPathsDataFrame(RelationalSchema const* relation) : relation(relation){};

    Vertical CreateVertical(std::vector<model::ColumnIndex> const& indices) {
        boost::dynamic_bitset<> ind_bitset(relation->GetNumColumns());
        for (auto const& index : indices) ind_bitset.set(index);

        return relation->GetVertical(ind_bitset);
    }

    model::ND CreateNd(NDTuple const& nd_to_create) {
        boost::dynamic_bitset<> lhs_indices_(relation->GetNumColumns()),
                rhs_indices_(relation->GetNumColumns());
        auto const& [lhs, rhs, weight] = nd_to_create;

        return {ActiveNdPathsDataFrame::CreateVertical(lhs),
                ActiveNdPathsDataFrame::CreateVertical(rhs), weight};
    }

    model::NDPath CreateNdPath(std::set<NDTuple> const& nd_tuples, Vertical const& start) {
        std::set<model::ND> nds;
        for (auto const& nd : nd_tuples) nds.emplace(ActiveNdPathsDataFrame::CreateNd(nd));

        return {nds, start};
    }
};

struct ActiveNdPathsParams {
    config::InputTable input_table;
    std::vector<std::set<NDTuple>> nd_paths;
    std::vector<model::ColumnIndex> start_indices;
    std::vector<model::ColumnIndex> end_indices;
    bool null_eq_null;

    ActiveNdPathsParams(config::InputTable input_table, std::vector<std::set<NDTuple>> nd_paths,
                        std::vector<model::ColumnIndex> start_indices,
                        std::vector<model::ColumnIndex> end_indices, bool null_eq_null = true)
        : input_table(std::move(input_table)),
          nd_paths(nd_paths),
          start_indices(start_indices),
          end_indices(end_indices),
          null_eq_null(null_eq_null) {}
};

class TestActiveNdPaths : public ::testing::TestWithParam<ActiveNdPathsParams> {};

TEST_P(TestActiveNdPaths, DefaultTest) {
    auto const& p = GetParam();
    auto end_indices = p.end_indices;
    auto start_indices = p.start_indices;
    auto input_table = p.input_table;
    auto null_eq_null = p.null_eq_null;
    auto nd_paths = p.nd_paths;
    std::vector<std::set<NDTuple>> expected_order = {{{{1, 5}, {2, 3}, 10}, {{1, 6}, {4, 5}, 10}},
                                                     {{{0, 1}, {3}, 8}, {{1}, {0, 4}, 9}},
                                                     {{{1}, {0, 5}, 3}},
                                                     {{{0, 1}, {5}, 5}}};

    auto relation = ColumnLayoutRelationData::CreateFrom(*input_table, null_eq_null);
    input_table->Reset();
    ActiveNdPathsDataFrame data_frame((*relation).GetSchema());

    Vertical end = data_frame.CreateVertical(end_indices);
    Vertical start = data_frame.CreateVertical(start_indices);

    algos::nd::util::ActiveNdPaths<algos::nd::util::BeFComparator> nd_queue(end);

    for (auto const& nd_path : nd_paths) {
        nd_queue.Push(data_frame.CreateNdPath(nd_path, start));
    }
    std::vector<std::set<NDTuple>> result;
    while (nd_queue.IsEmpty() == false) {
        auto res = nd_queue.Pop();
        result.push_back(NDsToTuples(res.NDs()));
    }

    EXPECT_EQ(result, expected_order);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    NDMiningTestsBuildInitialGraph, TestBuildInitialGraph,
    ::testing::Values(
        // Simple example from NDVerifier test suite:
        BuildInitialGraphParams(kTestNDInputTable, kTestNDNDs, true)
        ));

INSTANTIATE_TEST_SUITE_P(
    NDMiningTestsActiveNdPaths, TestActiveNdPaths,
    ::testing::Values(
        // Simple example from NDVerifier test suite:
        ActiveNdPathsParams(kTestNDInputTable, kTestNd_Paths, {0}, {3, 4, 5})
        ));

// clang-format on

class TestBbndAlgorithm : public ::testing::Test {
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
