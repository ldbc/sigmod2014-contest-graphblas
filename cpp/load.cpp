#include <fstream>
#include <ctime>
#include <memory>
#include <iostream>
#include <iomanip>
#include "load.h"
#include <cassert>
#include "utils.h"
#include "csv.h"

template<char Delimiter>
struct ignore_until {
};

constexpr ignore_until<'\n'> ignore_line;
constexpr ignore_until<'|'> ignore_field;

template<typename _CharT, typename _Traits, char Delimiter>
std::basic_istream<_CharT, _Traits> &
operator>>(std::basic_istream<_CharT, _Traits> &stream, const ignore_until<Delimiter> &) {
    return stream.ignore(std::numeric_limits<std::streamsize>::max(), Delimiter);
}

bool read_comment_or_post_line(std::ifstream &file, uint64_t &id, time_t &timestamp) {
    char delimiter;
    const char *timestamp_format = "%Y-%m-%d %H:%M:%S";

    std::tm t = {};
    if (!(file >> id >> delimiter >> std::get_time(&t, timestamp_format) >> ignore_line))
        return false;

    // depending on current time zone
    // it's acceptable since we only use these for comparison
    timestamp = std::mktime(&t);

    return true;
}

bool read_comment_line(std::ifstream &comments_file, Q2_Input &input, GrB_Index &comment_col) {
    uint64_t comment_id;
    time_t timestamp;
    if (!read_comment_or_post_line(comments_file, comment_id, timestamp))
        return false;

    comment_col = input.comments.size();
    input.comment_id_to_column.emplace(comment_id, comment_col);
    input.comments.emplace_back(comment_id, timestamp);

    return true;
}

bool read_comment_line(std::ifstream &comments_file, Q2_Input &input) {
    GrB_Index comment_col;
    return read_comment_line(comments_file, input, comment_col);
}

bool read_friends_line(GrB_Index &user1_column, GrB_Index &user2_column, std::ifstream &friends_file, Q2_Input &input) {
    char delimiter;
    uint64_t user1_id, user2_id;
    if (!(friends_file >> user1_id >> delimiter >> user2_id))
        return false;

    user1_column = input.user_id_to_column.emplace(user1_id, input.user_id_to_column.size()).first->second;
    user2_column = input.user_id_to_column.emplace(user2_id, input.user_id_to_column.size()).first->second;

    return true;
}

bool read_likes_line(GrB_Index &user_column, GrB_Index &comment_column, std::ifstream &likes_file, Q2_Input &input) {
    char delimiter;
    uint64_t user_id, comment_id;
    if (!(likes_file >> user_id >> delimiter >> comment_id))
        return false;

    // new users can be added here, since friends file only added users with friends
    user_column = input.user_id_to_column.emplace(user_id, input.user_id_to_column.size())
            .first->second;
    comment_column = input.comment_id_to_column.find(comment_id)->second;

    return true;
}

Q2_Input Q2_Input::load_initial(const BenchmarkParameters &parameters) {
    std::string comments_path = parameters.ChangePath + "/tag.csv";
    std::string friends_path = parameters.ChangePath + "/person_knows_person.csv";
    std::string likes_path = parameters.ChangePath + "/person_hasInterest_tag.csv";

    std::ifstream
            comments_file{comments_path},
            friends_file{friends_path},
            likes_file{likes_path};
    if (!(comments_file && friends_file && likes_file)) {
        throw std::runtime_error{"Failed to open input files"};
    }

    VertexCollection<Vertex> coll{comments_path};

    return {};

    Q2_Input input;

    while (read_comment_line(comments_file, input));

    std::vector<GrB_Index> friends_src_columns, friends_trg_columns;
    GrB_Index user1_column, user2_column;
    while (read_friends_line(user1_column, user2_column, friends_file, input)) {
        friends_src_columns.emplace_back(user1_column);
        friends_trg_columns.emplace_back(user2_column);
    }

    std::vector<GrB_Index> likes_src_user_columns, likes_trg_comment_columns;
    GrB_Index user_column, comment_column;
    while (read_likes_line(user_column, comment_column, likes_file, input)) {
        likes_src_user_columns.emplace_back(user_column);
        likes_trg_comment_columns.emplace_back(comment_column);
    }

    input.likes_num = likes_src_user_columns.size();

    input.likes_matrix_tran = GB(GrB_Matrix_new, GrB_BOOL, input.comments_size(), input.users_size());
    ok(GrB_Matrix_build_BOOL(input.likes_matrix_tran.get(),
                             likes_trg_comment_columns.data(), likes_src_user_columns.data(),
                             array_of_true(input.likes_num).get(),
                             input.likes_num, GrB_LOR));

    input.friends_num = friends_src_columns.size();

    input.friends_matrix = GB(GrB_Matrix_new, GrB_BOOL, input.users_size(), input.users_size());
    ok(GrB_Matrix_build_BOOL(input.friends_matrix.get(),
                             friends_src_columns.data(), friends_trg_columns.data(),
                             array_of_true(input.friends_num).get(),
                             input.friends_num, GrB_LOR));

    // make sure tuples are in row-major order (SuiteSparse extension)
    GxB_Format_Value format;
    ok(GxB_Matrix_Option_get(input.likes_matrix_tran.get(), GxB_FORMAT, &format));
    if (format != GxB_BY_ROW) {
        throw std::runtime_error{"Matrix is not CSR"};
    }

    return input;
}
