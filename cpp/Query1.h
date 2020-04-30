#pragma once

#include <queue>
#include <algorithm>
#include <cassert>
#include <numeric>
#include <memory>
#include <set>
#include <cstdio>
#include <utility>
#include "utils.h"
#include "Query.h"
#include <cstdio>

class Query1 : public Query<uint64_t, uint64_t, int> {
    GrB_Index p1, p2;
    int comment_lower_limit;

    std::tuple<std::string, std::string> initial_calculation() override {
        
        
        GBxx_Object<GrB_Matrix> personAToComment2 = GB(GrB_Matrix_new, GrB_UINT64, input.person_size(), input.comment_size());
        ok(GrB_mxm(personAToComment2.get(), GrB_NULL, GrB_NULL, GxB_PLUS_TIMES_INT64, input.hasCreatorTran.matrix.get(), input.replyOf.matrix.get(), GrB_NULL));
        ok(GxB_Matrix_fprint(personAToComment2.get(), "personAToComment2", GxB_SUMMARY, stdout));

        GBxx_Object<GrB_Matrix> personToPerson = GB(GrB_Matrix_new, GrB_UINT64, input.person_size(), input.person_size());

        ok(GrB_mxm(personToPerson.get(), input.knows.matrix.get(), GrB_NULL, GxB_PLUS_TIMES_INT64, personAToComment2.get(), input.hasCreator.matrix.get(), GrB_NULL));
        ok(GxB_Matrix_fprint(personToPerson.get(), "personToPerson", GxB_SUMMARY, stdout));

        auto limit = GB(GxB_Scalar_new, GrB_INT8);
        ok(GxB_Scalar_setElement_INT8(limit.get(), comment_lower_limit));



        ok(GxB_Matrix_select(personToPerson.get(), GrB_NULL, GrB_NULL, GxB_GT_THUNK, personToPerson.get(), limit.get(), GrB_NULL));
        ok(GxB_Matrix_fprint(personToPerson.get(), "personToPersonFiltered", GxB_SUMMARY, stdout));

        //Person to person is symmtetric, so no need to transpose
        GBxx_Object<GrB_Vector> v_output = GB(LAGraph_bfs_pushpull, nullptr, personToPerson.get(), personToPerson.get(), p1, GrB_NULL, false);
        //ok(GrB_Matrix_apply(personToPerson.get(), GrB_NULL, unaryop, personToPerson.get(), GrB_NULL));

        ok(GxB_Vector_fprint(v_output.get(), "output_vec", GxB_SUMMARY, stdout));

        std::string result_str, comment_str;
        return {result_str, comment_str};
    }

public:
    Query1(BenchmarkParameters parameters, ParameterType query_params, QueryInput const &input)
            : Query(std::move(parameters), std::move(query_params), input) {
        uint64_t p1_id, p2_id;

        std::tie(p1_id, p2_id, comment_lower_limit) = queryParams;

        p1 = input.persons.id_to_index.find(p1_id)->second;
        p2 = input.persons.id_to_index.find(p2_id)->second;
    }
};
