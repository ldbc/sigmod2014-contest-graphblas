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

class Query3 : public Query<int, int, std::string> {
    int topKLimit, maximumHopCount;
    std::string placeName;

    GBxx_Object<GrB_Vector> getRelevantPersons() {
        GrB_Index place_index = input.places.findIndexByName(placeName);

        // set starting place
        auto selected_places = GB(GrB_Vector_new, GrB_BOOL, input.places.size());
        ok(GrB_Vector_setElement_BOOL(selected_places.get(), true, place_index));

        auto selected_organizations = GB(GrB_Vector_new, GrB_BOOL, input.organizations.size());
        auto relevant_persons = GB(GrB_Vector_new, GrB_BOOL, input.persons.size());

        Places::Type place_type = input.places.types[place_index];
        while (true) {
            switch (place_type) {
                case Places::Continent:
                    // no person to reach from the continent
                    break;
                case Places::Country:
                    // get companies
                    ok(GrB_vxm(selected_organizations.get(), GrB_NULL, GrB_NULL, GxB_ANY_PAIR_BOOL,
                               selected_places.get(), input.organizationIsLocatedInPlaceTran.matrix.get(), GrB_NULL));
                    // get persons working at companies
                    ok(GrB_vxm(relevant_persons.get(), GrB_NULL, GrB_NULL, GxB_ANY_PAIR_BOOL,
                               selected_organizations.get(), input.workAtTran.matrix.get(), GrB_NULL));
                    break;
                case Places::City:
                    // get universities
                    ok(GrB_vxm(selected_organizations.get(), GrB_NULL, GrB_NULL, GxB_ANY_PAIR_BOOL,
                               selected_places.get(), input.organizationIsLocatedInPlaceTran.matrix.get(), GrB_NULL));
                    // add persons studying at universities
                    ok(GrB_vxm(relevant_persons.get(), GrB_NULL, GxB_PAIR_BOOL, GxB_ANY_PAIR_BOOL,
                               selected_organizations.get(), input.studyAtTran.matrix.get(), GrB_NULL));

                    // add persons at cities
                    ok(GrB_vxm(relevant_persons.get(), GrB_NULL, GxB_PAIR_BOOL, GxB_ANY_PAIR_BOOL,
                               selected_places.get(), input.personIsLocatedInCityTran.matrix.get(), GrB_NULL));
                    break;
                default:
                    throw std::runtime_error("Unknown Place.type");
            }

            // get parts of current places (overwriting current ones)
            if (place_type < Places::City) {
                ok(GrB_vxm(selected_places.get(), GrB_NULL, GrB_NULL,
                           GxB_ANY_PAIR_BOOL, selected_places.get(), input.isPartOfTran.matrix.get(), GrB_NULL));
                ++place_type;
            } else
                break;
        }

        return relevant_persons;
    }

    std::tuple<std::string, std::string> initial_calculation() override {
        auto relevant_persons = getRelevantPersons();

        ok(GxB_Vector_fprint(relevant_persons.get(), "relevant_persons", GxB_SHORT, stdout));

        std::string result_str, comment_str;
        return {result_str, comment_str};
    }

public:
    Query3(BenchmarkParameters parameters, ParameterType query_params, QueryInput const &input)
            : Query(std::move(parameters), std::move(query_params), input) {
        std::tie(topKLimit, maximumHopCount, placeName) = queryParams;
    }
};
