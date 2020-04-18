#include <fstream>
#include <ctime>
#include <memory>
#include <iostream>
#include <iomanip>
#include "load.h"
#include <cassert>
#include "utils.h"
#include "csv.h"

/*
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

bool read_friends_line(GrB_Index &user1_column, GrB_Index &user2_column, std::ifstream &friends_file, Q2_Input &input) {
    char delimiter;
    uint64_t user1_id, user2_id;
    if (!(friends_file >> user1_id >> delimiter >> user2_id))
        return false;

    user1_column = input.user_id_to_column.emplace(user1_id, input.user_id_to_column.size()).first->second;
    user2_column = input.user_id_to_column.emplace(user2_id, input.user_id_to_column.size()).first->second;

    return true;
}

bool read_comment_line(std::ifstream &comments_file, Q2_Input &input) {
    GrB_Index comment_col;
    return read_comment_line(comments_file, input, comment_col);
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
*/

std::tuple<std::ifstream, std::vector<std::string>, std::string> openFileWithHeader(const std::string &file_path) {
    std::ifstream csv_file{file_path};
    if (!csv_file)
        throw std::runtime_error{"Failed to open input file at: " + file_path};

    std::string header_line;
    if (!std::getline(csv_file, header_line)) {
        throw std::runtime_error{"Failed to read header: " + file_path};
    }

    std::vector<std::string> full_column_names;
    std::istringstream header_stream(header_line);

    std::string field_value;
    while (std::getline(header_stream, field_value, '|')) {
        full_column_names.emplace_back(field_value);
    }

    // Seek back the file
    csv_file.seekg(0);

    return {std::move(csv_file), std::move(full_column_names), std::move(header_line)};
}

std::string parseHeaderField(std::string const &field, char const *prefix, char const *postfix) {
    size_t field_len = field.size();
    size_t prefix_len = strlen(prefix);
    size_t postfix_len = strlen(postfix);

    assert(field.compare(0, prefix_len, prefix) == 0
           && field.compare(field_len - postfix_len, postfix_len, postfix) == 0);
    return field.substr(prefix_len, field_len - prefix_len - postfix_len);
}

const BaseVertexCollection &EdgeCollection::findVertexCollection(const std::string &vertex_name,
                                                                 const std::vector<std::reference_wrapper<BaseVertexCollection>> &vertex_collection) {
    auto iterator = std::find_if(vertex_collection.begin(), vertex_collection.end(),
                                 [&](const BaseVertexCollection &vertex) {
                                     return vertex.vertex_name == vertex_name;
                                 });

    if (iterator == vertex_collection.end())
        throw std::invalid_argument{"Vertex is not loaded: " + vertex_name};

    return *iterator;
}
