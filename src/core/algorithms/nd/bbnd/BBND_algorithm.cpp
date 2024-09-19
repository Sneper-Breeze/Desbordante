#include "algorithms/nd/bbnd/BBND_algorithm.h"
#include <boost/unordered_map.hpp>

#include <chrono>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <easylogging++.h>

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
#include "algorithms/nd/util/build_initial_graph.h"


namespace algos {
Bbnd::Bbnd() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::kEqualNullsOpt.GetName()});
}

void Bbnd::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kEqualNullsOpt(&is_null_equal_null_));
}

void Bbnd::LoadDataInternal(){
    relation_ =
            ColumnLayoutRelationData::CreateFrom(*input_table_, is_null_equal_null_);
    input_table_->Reset();
    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: ND mining is meaningless.");
    }

    graph_ = std::make_shared<model::NDGraph>(nd::util::BuildInitialGraph(*relation_));
}

void Bbnd::MakeExecuteOptsAvailable(){
    // Add here any specific execution opts to be available in future
}

unsigned long long Bbnd::ExecuteInternal() {
    // Alghorithm will be here
    unsigned long long execution_time = 0;

    return execution_time;
}

} // namespace algos
