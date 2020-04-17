#pragma once

#include <fstream>
#include <ctime>
#include <memory>
#include <iostream>
#include <iomanip>
#include "load.h"
#include <cassert>
#include <map>
#include <vector>
#include "utils.h"
#include "csv.h"
#include "gb_utils.h"

template<typename T>
struct VertexCollection {
    std::string vertex_name;
    std::vector<T> vertices;
    std::map<uint64_t, GrB_Index> id_to_index;

    VertexCollection(const std::string &file_path) {
        importFile(file_path);
    }

    void importFile(const std::string &file_path) {
        std::ifstream csv_file{file_path};
        if (!csv_file)
            throw std::runtime_error{"Failed to open input file at: " + file_path};

        std::vector<std::string> full_column_names;
        std::string header_line;
        if (!std::getline(csv_file, header_line))
            throw std::runtime_error{"Failed to read header: " + file_path};

        std::cout << header_line << std::endl;

        std::istringstream header_stream(header_line);
        std::string field_value;
        while (std::getline(header_stream, field_value, '|')) {
            full_column_names.emplace_back(field_value);
        }

        csv_file.seekg(0);

        auto extra_columns_array = T::extraColumns();
        constexpr auto selected_columns_count = extra_columns_array.size() + 1;

        std::array<std::string, selected_columns_count> selected_column_names;
        selected_column_names[0] = "id";
        std::copy(extra_columns_array.begin(), extra_columns_array.end(), selected_column_names.begin() + 1);

        for (auto &col_name : selected_column_names) {
            auto iterator = std::find_if(full_column_names.begin(), full_column_names.end(),
                                         [&](const auto &full_name) {
                                             // find column name with optional type suffix (e.g. "name" or "name:STRING")
                                             return full_name.rfind(col_name, 0) == 0
                                                    && (col_name.size() == full_name.size() ||
                                                        full_name[col_name.size()] == ':');
                                         });
            if (iterator == full_column_names.end())
                throw std::invalid_argument{
                        "Cannot find column: " + col_name + "\nAvailable columns: " +
                        header_line};
            col_name = *iterator;
        }

        std::string id_column = selected_column_names[0];

        std::string id_prefix = "id:ID(";
        char id_postfix = ')';
        assert(id_column.compare(0, id_prefix.size(), id_prefix) == 0
               && id_column[id_column.size() - 1] == id_postfix);
        vertex_name = id_column.substr(id_prefix.size(), id_column.size() - id_prefix.size() - 1);

        io::CSVReader<selected_columns_count, io::trim_chars<>, io::no_quote_escape<'|'>> csv_reader(file_path,
                                                                                                     csv_file);

        std::apply([&](auto... col) { csv_reader.read_header(io::ignore_extra_column, col...); },
                   selected_column_names);

        T vertex;
        while (vertex.parseLine(csv_reader)) {
            GrB_Index current_index = vertices.size();

            vertices.emplace_back(std::move(vertex));
            bool newly_inserted = id_to_index.insert({vertex.id, current_index}).second;
            assert(newly_inserted);
        }
    }
};

struct Vertex {
    uint64_t id;

    static auto extraColumns() {
        return array_of<char const *>();
    }

    template<typename Reader>
    bool parseLine(Reader &csv_reader) {
        return csv_reader.read_row(id);
    }
};

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"

struct Tag : public Vertex {
    std::string name;

    static auto extraColumns() {
        return array_of<char const *>("name");
    }

    template<typename Reader>
    bool parseLine(Reader &csv_reader) {
        return csv_reader.read_row(id, name);
    }
};

struct Person : public Vertex {
    time_t birthday;

    static auto extraColumns() {
        return array_of<char const *>("birthday");
    }

    template<typename Reader>
    bool parseLine(Reader &csv_reader) {
        const char *birthday_str = nullptr;
        if (!csv_reader.read_row(id, birthday_str))
            return false;

        birthday = parseTimestamp(birthday_str, DateFormat);

        return true;
    }
};

#pragma clang diagnostic pop

struct Q2_Input {
    /*
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
    }*/

    explicit Q2_Input(const BenchmarkParameters &parameters) {
        std::string knows_path = parameters.ChangePath + "person_knows_person.csv";
        std::string likes_path = parameters.ChangePath + "person_hasInterest_tag.csv";

        std::ifstream
                comments_file{(parameters.ChangePath + "/tag.csv")},
                friends_file{knows_path},
                likes_file{likes_path};
        if (!(comments_file && friends_file && likes_file)) {
            throw std::runtime_error{"Failed to open input files"};
        }

        VertexCollection<Tag> tags{(parameters.ChangePath + "tag.csv")};
        VertexCollection<Person> persons{(parameters.ChangePath + "person.csv")};


/*
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

        return input;*/
    }
};
