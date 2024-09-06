#pragma once

#include <set>

#include "algorithms/nd/nd.h"
#include "model/table/column_layout_relation_data.h"

namespace algos::nd::util {

std::set<model::ND> BuildInitialGraph(ColumnLayoutRelationData const& data);

}  // namespace algos::nd::util
