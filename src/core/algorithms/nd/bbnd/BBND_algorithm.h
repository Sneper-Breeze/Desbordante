#pragma once

#include <list>
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/nd/model/nd_graph.h"
#include "algorithms/nd/model/nd_path.h"
#include "algorithms/nd/nd.h"
#include "config/equal_nulls/type.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_relation_data.h"
#include "util/primitive_collection.h"

namespace algos {

class Bbnd : public Algorithm {
private:
    config::InputTable input_table_;
    config::EqNullsType is_null_equal_null_;
    util::PrimitiveCollection<model::ND> nd_collection_;
    std::shared_ptr<ColumnLayoutRelationData> relation_;
    std::vector<model::NDPath> queue_;
    std::shared_ptr<model::NDGraph> graph_ = nullptr;

    void RegisterOptions();
    void ResetState() override{};
    void MakeExecuteOptsAvailable() override;
    void LoadDataInternal() override;
    unsigned long long ExecuteInternal() override;

public:
    Bbnd();

    /* Returns the list of discovered NDs */
    std::list<model::ND> const& NdList() const noexcept {
        return nd_collection_.AsList();
    }

    std::list<model::ND>& NdList() noexcept {
        return nd_collection_.AsList();
    }

    // virtual ~Bbnd() = default;
};
}  // namespace algos
