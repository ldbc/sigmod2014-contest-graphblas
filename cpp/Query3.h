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

    GBxx_Object<GrB_Matrix> hasInterest;

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

    using score_type = std::tuple<int64_t, uint64_t, uint64_t>;

    void tagCount_filtered_reachable_count_tags_strategy(GrB_Vector const local_persons,
                                                         SmallestElementsContainer<score_type, std::less<score_type>> &person_scores) {
        // maximum value: 10 -> UINT8
        auto tag_count_per_person = GB(GrB_Vector_new, GrB_UINT8, input.persons.size());

        ok(GrB_Vector_assign_UINT8(tag_count_per_person.get(), local_persons, GrB_NULL, 0, GrB_ALL, 0, GrB_NULL));
        ok(GrB_Matrix_reduce_Monoid(tag_count_per_person.get(), local_persons, GrB_NULL, GrB_PLUS_MONOID_UINT8,
                                    hasInterest.get(), GrB_NULL));

        uint8_t max_tag_count;
        ok(GrB_Vector_reduce_UINT8(&max_tag_count, GrB_NULL, GrB_MAX_MONOID_UINT8, tag_count_per_person.get(),
                                   GrB_NULL));
#ifndef NDEBUG
        std::cerr << "max_tag_count: " << (unsigned) max_tag_count << std::endl;
#endif

        GrB_Index common_interests_nvals;
        auto relevant_persons = GB(GrB_Vector_new, GrB_BOOL, input.persons.size());
        GBxx_Object<GrB_Matrix> common_interests;

        for (int lower_tag_count = max_tag_count;;) {
            // add persons with less tags
            auto limit = GB(GxB_Scalar_new, GrB_UINT8);
            ok(GxB_Scalar_setElement_INT32(limit.get(), lower_tag_count));
            ok(GxB_Vector_select(relevant_persons.get(), GrB_NULL, GxB_PAIR_BOOL, GxB_EQ_THUNK,
                                 tag_count_per_person.get(),
                                 limit.get(), GrB_NULL));
            GrB_Index relevant_persons_nvals;
            ok(GrB_Vector_nvals(&relevant_persons_nvals, relevant_persons.get()));

            std::vector<GrB_Index> relevant_persons_indices(relevant_persons_nvals);
            {
                GrB_Index nvals = relevant_persons_nvals;
                ok(GrB_Vector_extractTuples_BOOL(relevant_persons_indices.data(), GrB_NULL, &nvals,
                                                 relevant_persons.get()));
                assert(relevant_persons_nvals == nvals);
            }

            // build diagonal matrix of relevant persons
            auto persons_diag_mx = GB(GrB_Matrix_new, GrB_BOOL, input.persons.size(), input.persons.size());
            ok(GrB_Matrix_build_BOOL(persons_diag_mx.get(),
                                     relevant_persons_indices.data(), relevant_persons_indices.data(),
                                     array_of_true(relevant_persons_nvals).get(), relevant_persons_nvals,
                                     GxB_PAIR_BOOL));

            auto next_mx = GB(GrB_Matrix_dup, persons_diag_mx.get());
            auto seen_mx = GB(GrB_Matrix_dup, next_mx.get());

            // MSBFS from relevant persons
            for (int i = 0; i < maximumHopCount; ++i) {
                ok(GrB_mxm(next_mx.get(), seen_mx.get(), GrB_NULL, GxB_ANY_PAIR_BOOL, next_mx.get(),
                           input.knows.matrix.get(), GrB_DESC_RSC));

                GrB_Index next_mx_nvals;
                ok(GrB_Matrix_nvals(&next_mx_nvals, next_mx.get()));
                // if emptied the component
                if (next_mx_nvals == 0)
                    break;

                ok(GrB_Matrix_eWiseAdd_BinaryOp(seen_mx.get(), GrB_NULL, GrB_NULL,
                                                GxB_PAIR_BOOL, seen_mx.get(), next_mx.get(), GrB_NULL));
            }

            // strictly lower triangular matrix is enough for reachable persons
            // source persons were filtered at the beginning
            // drop friends in different place
            ok(GxB_Matrix_select(seen_mx.get(), GrB_NULL, GrB_NULL, GxB_OFFDIAG, seen_mx.get(), GrB_NULL, GrB_NULL));
            ok(GxB_Matrix_select(seen_mx.get(), GrB_NULL, GrB_NULL, GxB_TRIL, seen_mx.get(), GrB_NULL, GrB_NULL));
            ok(GrB_mxm(seen_mx.get(), GrB_NULL, GrB_NULL, GxB_ANY_PAIR_BOOL, seen_mx.get(), persons_diag_mx.get(),
                       GrB_NULL));
            auto h_reachable_knows_tril = std::move(seen_mx);

            // calculate common interests between persons in h hop distance
            common_interests = GB(GrB_Matrix_new, GrB_INT64, input.persons.size(), input.persons.size());
            ok(GrB_mxm(common_interests.get(), h_reachable_knows_tril.get(), GrB_NULL, GxB_PLUS_TIMES_INT64,
                       hasInterest.get(), input.hasInterestTran.matrix.get(), GrB_DESC_S));

            // count tag scores per person pairs
            ok(GrB_Matrix_nvals(&common_interests_nvals, common_interests.get()));
            if (common_interests_nvals < topKLimit) {
                // there are not enough non-zero scores
                // add reachable persons with zero common tags
                // assign 0 to every reachable person pair, but select non-zero score (first operand) if present
                // common_interests <h_reachable_knows_tril> 1ST= 0
                ok(GrB_Matrix_assign_INT64(common_interests.get(), h_reachable_knows_tril.get(), GrB_FIRST_INT64,
                                           0, GrB_ALL, 0, GrB_ALL, 0, GrB_DESC_S));

                // recount nvals
                ok(GrB_Matrix_nvals(&common_interests_nvals, common_interests.get()));
            }

            // extract result from matrix
            std::vector<GrB_Index> common_interests_rows(common_interests_nvals),
                    common_interests_cols(common_interests_nvals);
            std::vector<int64_t> common_interests_vals(common_interests_nvals);
            {
                GrB_Index nvals = common_interests_nvals;
                ok(GrB_Matrix_extractTuples_INT64(common_interests_rows.data(), common_interests_cols.data(),
                                                  common_interests_vals.data(), &nvals, common_interests.get()));
                assert(common_interests_nvals == nvals);
            }

            person_scores.removeElements();
            // collect top scores
            for (size_t i = 0; i < common_interests_vals.size(); ++i) {
                GrB_Index p1_index = common_interests_rows[i],
                        p2_index = common_interests_cols[i];
                int64_t score = common_interests_vals[i];

                uint64_t p1_id = input.persons.vertexIds[p1_index];
                uint64_t p2_id = input.persons.vertexIds[p2_index];
                // put the smallest ID first
                if (p1_id > p2_id)
                    std::swap(p1_id, p2_id);

                // DESC score
                person_scores.add({-score, p1_id, p2_id});
            }

            if (person_scores.size() == topKLimit && std::get<0>(person_scores.max()) == -lower_tag_count) {
#ifndef NDEBUG
                std::cerr << "stopped at min(top scores)=" << lower_tag_count << std::endl;
#endif
                break;
            }

            if (lower_tag_count != 0)
                --lower_tag_count;
            else
                break;
        }
    }

    inline __attribute__((always_inline))
    void push_next(GrB_Matrix Next, GrB_Matrix Seen, GrB_Matrix Edges) {
        // next<!seen> = next * A
        ok(GrB_mxm(Next, Seen, NULL, GxB_ANY_PAIR_BOOL, Next, Edges, GrB_DESC_RC));

        // seen pair= next
        ok(GrB_transpose(Seen, NULL, GxB_PAIR_BOOL, Next, GrB_DESC_T0));
    }

    void tagCount_msbfs_strategy(GrB_Vector const local_persons,
                                 SmallestElementsContainer<score_type, std::less<score_type>> &person_scores) {
        // maximum value: 10 -> UINT8
        auto tag_count_per_person = GB(GrB_Vector_new, GrB_UINT8, input.persons.size());

        // every local person has at least 0 tags
        ok(GrB_Vector_assign_UINT8(tag_count_per_person.get(), local_persons, GrB_NULL, 0, GrB_ALL, 0, GrB_NULL));
        // count tags per person
        ok(GrB_Matrix_reduce_Monoid(tag_count_per_person.get(), local_persons, GrB_NULL, GrB_PLUS_MONOID_UINT8,
                                    hasInterest.get(), GrB_NULL));

        uint8_t max_tag_count;
        ok(GrB_Vector_reduce_UINT8(&max_tag_count, GrB_NULL, GrB_MAX_MONOID_UINT8, tag_count_per_person.get(),
                                   GrB_NULL));
#ifndef NDEBUG
        std::cerr << "max_tag_count: " << (unsigned) max_tag_count << std::endl;
#endif

        // persons with 10 tags, persons with 9..10 tags, ...
        auto relevant_persons = GB(GrB_Vector_new, GrB_BOOL, input.persons.size());
        for (int lower_tag_count = max_tag_count;;) {
#ifndef NDEBUG
            std::cerr << "Loop:" << lower_tag_count << std::endl;
#endif
            // add persons with less tags
            auto limit = GB(GxB_Scalar_new, GrB_UINT8);
            ok(GxB_Scalar_setElement_INT32(limit.get(), lower_tag_count));
            ok(GxB_Vector_select(relevant_persons.get(), GrB_NULL, GxB_PAIR_BOOL, GxB_EQ_THUNK,
                                 tag_count_per_person.get(),
                                 limit.get(), GrB_NULL));
            GrB_Index relevant_persons_nvals;
            ok(GrB_Vector_nvals(&relevant_persons_nvals, relevant_persons.get()));

            // extract relevant person indices
            std::vector<GrB_Index> relevant_persons_indices(relevant_persons_nvals);
            {
                GrB_Index nvals = relevant_persons_nvals;
                ok(GrB_Vector_extractTuples_BOOL(relevant_persons_indices.data(), GrB_NULL, &nvals,
                                                 relevant_persons.get()));
                assert(relevant_persons_nvals == nvals);
            }

            // build diagonal matrix of relevant persons
            auto persons_diag_mx = GB(GrB_Matrix_new, GrB_BOOL, input.persons.size(), input.persons.size());
            ok(GrB_Matrix_build_BOOL(persons_diag_mx.get(),
                                     relevant_persons_indices.data(), relevant_persons_indices.data(),
                                     array_of_true(relevant_persons_nvals).get(), relevant_persons_nvals,
                                     GxB_PAIR_BOOL));

            auto next_mx = GB(GrB_Matrix_dup, persons_diag_mx.get());
            auto seen_mx = GB(GrB_Matrix_dup, next_mx.get());

            // MSBFS from relevant persons
            // TODO: fix algorithm for odd maximumHopCount
            for (int i = 0; i < maximumHopCount / 2; ++i) {
                push_next(next_mx.get(), seen_mx.get(), input.knows.matrix.get());
            }

            // TODO: offdiag? tril?
//            ok(GxB_Matrix_select(seen_mx.get(), GrB_NULL, GrB_NULL, GxB_OFFDIAG, seen_mx.get(), GrB_NULL, GrB_NULL));
            auto half_reachable = std::move(seen_mx);

            // find vertices where relevant persons meet
            auto columns_where_vertices_meet = GB(GrB_Vector_new, GrB_UINT64, input.persons.size());
            ok(GrB_Matrix_reduce_Monoid(columns_where_vertices_meet.get(), GrB_NULL, GrB_NULL,
                                        GrB_PLUS_MONOID_UINT64, half_reachable.get(), GrB_DESC_T0));
#ifndef NDEBUG
            {
                GrB_Index nvals;
                ok(GrB_Vector_nvals(&nvals, columns_where_vertices_meet.get()));
                std::cerr << "columns_where_vertices_meet nvals:" << nvals << std::endl;
            }
#endif
            // keep vertices where at least 2 vertices meet
            auto scalar2 = GB(GxB_Scalar_new, GrB_UINT64);
            ok(GxB_Scalar_setElement_UINT64(scalar2.get(), 2));
            ok(GxB_Vector_select(columns_where_vertices_meet.get(), GrB_NULL, GrB_NULL, GxB_GE_THUNK,
                                 columns_where_vertices_meet.get(), scalar2.get(), GrB_NULL));

            // extract columns_where_vertices_meet
            GrB_Index columns_where_vertices_meet_nvals;
            ok(GrB_Vector_nvals(&columns_where_vertices_meet_nvals, columns_where_vertices_meet.get()));
            std::vector<GrB_Index> columns_where_vertices_meet_indices(columns_where_vertices_meet_nvals);
            {
                GrB_Index nvals = columns_where_vertices_meet_nvals;
                ok(GrB_Vector_extractTuples_UINT64(columns_where_vertices_meet_indices.data(), nullptr, &nvals,
                                                   columns_where_vertices_meet.get()));
                assert(columns_where_vertices_meet_nvals == nvals);
            }

#ifndef NDEBUG
            std::cerr << "columns_where_vertices_meet_nvals after select:" << columns_where_vertices_meet_nvals
                      << std::endl;
#endif

            // calculate common interests between persons in h hop distance
            GBxx_Object<GrB_Matrix> common_interests_global = GB(GrB_Matrix_new, GrB_UINT64, input.persons.size(),
                                                                 input.persons.size());

#pragma omp parallel num_threads(GlobalNThreads)
            {
                GBxx_Object<GrB_Matrix> common_interests = GB(GrB_Matrix_new, GrB_UINT64, input.persons.size(),
                                                              input.persons.size());
#pragma omp for schedule(static)
                for (GrB_Index i = 0; i < columns_where_vertices_meet_nvals; ++i) {
                    GrB_Index meet_column = columns_where_vertices_meet_indices[i];

                    // get persons who meet at vertex meet_column
                    auto meeting_vertices = GB(GrB_Vector_new, GrB_BOOL, input.persons.size());
                    ok(GrB_Col_extract(meeting_vertices.get(), GrB_NULL, GrB_NULL, half_reachable.get(), GrB_ALL,
                                       0, meet_column, GrB_NULL));
                    GrB_Index meeting_vertices_nvals;
                    ok(GrB_Vector_nvals(&meeting_vertices_nvals, meeting_vertices.get()));
                    std::vector<GrB_Index> meeting_vertices_indices(meeting_vertices_nvals);
                    {
                        GrB_Index nvals = meeting_vertices_nvals;
                        ok(GrB_Vector_extractTuples_UINT64(meeting_vertices_indices.data(), nullptr, &nvals,
                                                           meeting_vertices.get()));
                        assert(meeting_vertices_nvals == nvals);
                    }

                    for (GrB_Index p1_iter = 0; p1_iter < meeting_vertices_nvals; ++p1_iter) {
                        for (GrB_Index p2_iter = 0; p2_iter < p1_iter; ++p2_iter) {
                            GrB_Index p1 = meeting_vertices_indices[p1_iter];
                            GrB_Index p2 = meeting_vertices_indices[p2_iter];
                            ok(GrB_Matrix_setElement_UINT64(common_interests.get(), 0, p1, p2));
                        }
                    }
                }

#pragma omp critical(Q3_merge_thread_local_matrices)
                {
                    ok(GrB_transpose(common_interests_global.get(), GrB_NULL, GxB_PAIR_UINT64, common_interests.get(),
                                     GrB_DESC_T0));
                    auto ptr = common_interests_global.release();
                    ok(GrB_Matrix_wait(&ptr));
                    common_interests_global.reset(ptr);
                }
            }

            ok(GxB_Matrix_select(common_interests_global.get(), GrB_NULL, GrB_NULL,
                                 GxB_OFFDIAG, common_interests_global.get(), GrB_NULL, GrB_NULL));
            ok(GxB_Matrix_select(common_interests_global.get(), GrB_NULL, GrB_NULL,
                                 GxB_TRIL, common_interests_global.get(), GrB_NULL, GrB_NULL));

            auto common_interests_pattern = GB(GrB_Matrix_dup, common_interests_global.get());

            ok(GrB_mxm(common_interests_global.get(), common_interests_global.get(), GrB_NULL, GxB_PLUS_TIMES_UINT64,
                       hasInterest.get(), input.hasInterestTran.matrix.get(), GrB_DESC_S));

#ifndef NDEBUG
            ok(GxB_Matrix_fprint(common_interests_global.get(), "common_interests", GxB_SUMMARY, stdout));
#endif

            // count tag scores per person pairs
            GrB_Index common_interests_nvals;
            ok(GrB_Matrix_nvals(&common_interests_nvals, common_interests_global.get()));
            if (common_interests_nvals < topKLimit) {
                // there are not enough non-zero scores
                // add reachable persons with zero common tags
                // assign 0 to every reachable person pair, but select non-zero score (first operand) if present
                // common_interests <h_reachable_knows_tril> 1ST= 0
                ok(GrB_Matrix_assign_INT64(common_interests_global.get(),
                                           common_interests_pattern.get(), GrB_FIRST_INT64,
                                           0, GrB_ALL, 0, GrB_ALL, 0, GrB_DESC_S));

                // recount nvals
                ok(GrB_Matrix_nvals(&common_interests_nvals, common_interests_global.get()));
            }

            // extract result from matrix
            std::vector<GrB_Index> common_interests_rows(common_interests_nvals),
                    common_interests_cols(common_interests_nvals);
            std::vector<int64_t> common_interests_vals(common_interests_nvals);
            {
                GrB_Index nvals = common_interests_nvals;
                ok(GrB_Matrix_extractTuples_INT64(common_interests_rows.data(), common_interests_cols.data(),
                                                  common_interests_vals.data(), &nvals, common_interests_global.get()));
                assert(common_interests_nvals == nvals);
            }

            person_scores.removeElements();
            // collect top scores
            for (size_t i = 0; i < common_interests_vals.size(); ++i) {
                GrB_Index p1_index = common_interests_rows[i],
                        p2_index = common_interests_cols[i];
                int64_t score = common_interests_vals[i];

                uint64_t p1_id = input.persons.vertexIds[p1_index];
                uint64_t p2_id = input.persons.vertexIds[p2_index];
                // put the smallest ID first
                if (p1_id > p2_id)
                    std::swap(p1_id, p2_id);

                // DESC score
                person_scores.add({-score, p1_id, p2_id});
            }

            if (person_scores.size() == topKLimit && std::get<0>(person_scores.max()) == -lower_tag_count) {
#ifndef NDEBUG
                std::cerr << "stopped at min(top scores)=" << lower_tag_count << std::endl;
#endif
                break;
            }

            if (lower_tag_count != 0)
                --lower_tag_count;
            else
                break;
        }
    }

    void reachable_count_tags_strategy(GrB_Vector const local_persons,
                                       GrB_Index const local_persons_nvals,
                                       SmallestElementsContainer<score_type, std::less<score_type>> &person_scores) {
        std::vector<GrB_Index> local_persons_indices(local_persons_nvals);
        {
            GrB_Index nvals = local_persons_nvals;
            ok(GrB_Vector_extractTuples_BOOL(local_persons_indices.data(), GrB_NULL, &nvals,
                                             local_persons));
            assert(local_persons_nvals == nvals);
        }

        // build diagonal matrix of local persons
        auto persons_diag_mx = GB(GrB_Matrix_new, GrB_BOOL, input.persons.size(), input.persons.size());
        ok(GrB_Matrix_build_BOOL(persons_diag_mx.get(),
                                 local_persons_indices.data(), local_persons_indices.data(),
                                 array_of_true(local_persons_nvals).get(), local_persons_nvals, GxB_PAIR_BOOL));

        auto next_mx = GB(GrB_Matrix_dup, persons_diag_mx.get());
        auto seen_mx = GB(GrB_Matrix_dup, next_mx.get());

        // MSBFS from relevant persons
        for (int i = 0; i < maximumHopCount; ++i) {
            ok(GrB_mxm(next_mx.get(), seen_mx.get(), GrB_NULL, GxB_ANY_PAIR_BOOL, next_mx.get(),
                       input.knows.matrix.get(), GrB_DESC_RSC));

            GrB_Index next_mx_nvals;
            ok(GrB_Matrix_nvals(&next_mx_nvals, next_mx.get()));
            // if emptied the component
            if (next_mx_nvals == 0)
                break;

            ok(GrB_Matrix_eWiseAdd_BinaryOp(seen_mx.get(), GrB_NULL, GrB_NULL,
                                            GxB_PAIR_BOOL, seen_mx.get(), next_mx.get(), GrB_NULL));
        }

        // strictly lower triangular matrix is enough for reachable persons
        // source persons were filtered at the beginning
        // drop friends in different place
        ok(GxB_Matrix_select(seen_mx.get(), GrB_NULL, GrB_NULL, GxB_OFFDIAG, seen_mx.get(), GrB_NULL, GrB_NULL));
        ok(GxB_Matrix_select(seen_mx.get(), GrB_NULL, GrB_NULL, GxB_TRIL, seen_mx.get(), GrB_NULL, GrB_NULL));
        ok(GrB_mxm(seen_mx.get(), GrB_NULL, GrB_NULL, GxB_ANY_PAIR_BOOL, seen_mx.get(), persons_diag_mx.get(),
                   GrB_NULL));
        auto h_reachable_knows_tril = std::move(seen_mx);

        // calculate common interests between persons in h hop distance
        auto common_interests = GB(GrB_Matrix_new, GrB_INT64, input.persons.size(), input.persons.size());
        ok(GrB_mxm(common_interests.get(), h_reachable_knows_tril.get(), GrB_NULL, GxB_PLUS_TIMES_INT64,
                   hasInterest.get(), input.hasInterestTran.matrix.get(), GrB_DESC_S));

        // count tag scores per person pairs
        GrB_Index common_interests_nvals;
        ok(GrB_Matrix_nvals(&common_interests_nvals, common_interests.get()));

        if (common_interests_nvals < topKLimit) {
            // there are not enough non-zero scores
            // add reachable persons with zero common tags
            // assign 0 to every reachable person pair, but select non-zero score (first operand) if present
            // common_interests <h_reachable_knows_tril> 1ST= 0
            ok(GrB_Matrix_assign_INT64(common_interests.get(), h_reachable_knows_tril.get(), GrB_FIRST_INT64,
                                       0, GrB_ALL, 0, GrB_ALL, 0, GrB_DESC_S));

            // recount nvals
            ok(GrB_Matrix_nvals(&common_interests_nvals, common_interests.get()));
        }

        // extract result from matrix
        std::vector<GrB_Index> common_interests_rows(common_interests_nvals),
                common_interests_cols(common_interests_nvals);
        std::vector<int64_t> common_interests_vals(common_interests_nvals);
        {
            GrB_Index nvals = common_interests_nvals;
            ok(GrB_Matrix_extractTuples_INT64(common_interests_rows.data(), common_interests_cols.data(),
                                              common_interests_vals.data(), &nvals, common_interests.get()));
            assert(common_interests_nvals == nvals);
        }

        // collect top scores
        for (size_t i = 0; i < common_interests_vals.size(); ++i) {
            GrB_Index p1_index = common_interests_rows[i],
                    p2_index = common_interests_cols[i];
            int64_t score = common_interests_vals[i];

            uint64_t p1_id = input.persons.vertexIds[p1_index];
            uint64_t p2_id = input.persons.vertexIds[p2_index];
            // put the smallest ID first
            if (p1_id > p2_id)
                std::swap(p1_id, p2_id);

            // DESC score
            person_scores.add({-score, p1_id, p2_id});
        }
    }

    std::tuple<std::string, std::string> initial_calculation() override {
        hasInterest = GB(GrB_Matrix_new, GrB_BOOL, input.hasInterestTran.trg->size(),
                         input.hasInterestTran.src->size());
        ok(GrB_transpose(hasInterest.get(), GrB_NULL, GrB_NULL, input.hasInterestTran.matrix.get(), GrB_NULL));

        auto local_persons = getRelevantPersons();

        // extract person indices
        GrB_Index relevant_persons_nvals;
        ok(GrB_Vector_nvals(&relevant_persons_nvals, local_persons.get()));
        if (relevant_persons_nvals == 0)
            return {"", "Nobody lives/studies/works there."};

        auto person_scores = makeSmallestElementsContainer<score_type>(topKLimit);

//        reachable_count_tags_strategy(local_persons.get(), relevant_persons_nvals, person_scores);
//        tagCount_filtered_reachable_count_tags_strategy(local_persons.get(), person_scores);
        tagCount_msbfs_strategy(local_persons.get(), person_scores);

        std::string result, comment;
        bool firstIter = true;
        for (auto[neg_score, p1_id, p2_id]: person_scores.removeElements()) {
            if (firstIter)
                firstIter = false;
            else {
                result += ' ';
                comment += ' ';
            }

            result += std::to_string(p1_id);
            result += '|';
            result += std::to_string(p2_id);

            comment += std::to_string(-neg_score);
        }

        hasInterest.reset();

        return {result, comment};
    }

public:
    Query3(BenchmarkParameters parameters, ParameterType query_params, QueryInput const &input)
            : Query(std::move(parameters), std::move(query_params), input) {
        std::tie(topKLimit, maximumHopCount, placeName) = queryParams;
    }
};
