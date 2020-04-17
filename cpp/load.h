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
            std::cout << vertex.id << std::endl;
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

struct Post : public Vertex {
    time_t timestamp;

    Post(uint64_t post_id, time_t timestamp) : Vertex{post_id}, timestamp(timestamp) {}
};

struct Comment : public Vertex {
    time_t timestamp;

    Comment(uint64_t comment_id, time_t timestamp) : Vertex{comment_id}, timestamp(timestamp) {}
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
