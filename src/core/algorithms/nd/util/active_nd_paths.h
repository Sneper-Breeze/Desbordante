#pragma once

#include "algorithms/nd/nd.h"
#include <set>
#include <unordered_map>

#include "algorithms/nd/nd.h"
#include "algorithms/nd/model/nd_path.h"
#include "model/table/column_layout_relation_data.h"
#include "model/table/vertical.h"
#include "model/table/column.h"


namespace algos::nd::util{
int IntersectionWithEnd(model::NDPath nd_path, std::shared_ptr<std::set<Column>> end){
    auto attrs = nd_path.Attr();

    int ans = 0;
    auto it_end = end->begin();
    for(auto it_attr = attrs.begin(); it_attr != attrs.end(); it_attr++){
        while(it_end != end->end() && *it_end < *it_attr)
            it_end++;

        if(it_end != end->end() && *it_end == *it_attr)
            ans++;
    }

    return ans;
}


bool BeFCmpr(std::pair<model::NDPath, std::shared_ptr<std::set<Column>>> a,
             std::pair<model::NDPath, std::shared_ptr<std::set<Column>>> b){
    int res = IntersectionWithEnd(a.first, a.second) 
              - IntersectionWithEnd(b.first, b.second);

    if(res > 0)
        return true;
    if(res == 0 && a.first.Weight() < b.first.Weight())
        return true;

    return false;
}


template<typename _Compare = std::less<model::NDPath>>
class ActiveNdPaths {
    private:
        // int IntersectionWithEnd(model::NDPath nd_path);
        // bool CustomCmpr(model::NDPath a, model::NDPath b);
        
        std::set<std::pair<model::NDPath, std::shared_ptr<std::set<Column>>>, _Compare> queue_;
        std::shared_ptr<std::set<Column>> end_;

    public:
        ActiveNdPaths(Vertical end);
        ActiveNdPaths(std::set<Column> && end);
        
        // Changing methods
        model::NDPath Pop();
        void Push(model::NDPath && new_path);
        
        // Checkout methods
        inline bool IsEmpty() {
            return queue_.empty();
        };

};
} // namespace algos::nd::util