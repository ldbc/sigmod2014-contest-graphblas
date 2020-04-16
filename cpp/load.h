#pragma once

#include <vector>
#include <map>
#include "utils.h"
#include "gb_utils.h"

struct Post {
    uint64_t post_id;
    time_t timestamp;

    Post(uint64_t post_id, time_t timestamp) : post_id(post_id), timestamp(timestamp) {}
};

struct Comment {
    uint64_t comment_id;
    time_t timestamp;

    Comment(uint64_t comment_id, time_t timestamp) : comment_id(comment_id), timestamp(timestamp) {}
};

struct Q2_Input {
    std::vector<Comment> comments;
    std::map<uint64_t, GrB_Index> comment_id_to_column;

    std::map<uint64_t, GrB_Index> user_id_to_column;

    GBxx_Object<GrB_Matrix> likes_matrix_tran, friends_matrix;

    GrB_Index likes_num, friends_num;

    auto users_size() const {
        return user_id_to_column.size();
    }

    auto comments_size() const {
        return comments.size();
    }

    static Q2_Input load_initial(const BenchmarkParameters &parameters);
};
