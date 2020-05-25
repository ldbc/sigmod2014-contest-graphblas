#pragma once

#include <fstream>
#include <ctime>
#include <memory>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <map>
#include <utility>
#include <vector>
#include "utils.h"
#include "csv.h"
#include "gb_utils.h"

std::tuple<std::ifstream, std::vector<std::string>, std::string> openFileWithHeader(const std::string &file_path);

std::string parseHeaderField(std::string const &field, char const *prefix, char const *postfix);

struct BaseVertexCollection {
    std::string vertexName;
    std::vector<GrB_Index> vertexIds;
    GBxx_Object<GrB_Vector> idToIndex;

    GrB_Index size() const {
        return vertexIds.size();
    }
};

template<typename T>
struct VertexCollection : public BaseVertexCollection {
    std::vector<T> vertexExtraValues;

    VertexCollection(const std::string &file_path) {
        importFile(file_path);
    }

    void importFile(const std::string &file_path) {
        auto[csv_file, full_column_names, header_line] = openFileWithHeader(file_path);

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
        vertexName = parseHeaderField(id_column, "id:ID(", ")");

        io::CSVReader<selected_columns_count, io::trim_chars<>, io::no_quote_escape<'|'>> csv_reader(file_path,
                                                                                                     csv_file);

        std::apply([&](auto... col) { csv_reader.read_header(io::ignore_extra_column, col...); },
                   selected_column_names);

        T vertex_extra;
        GrB_Index id;
        while (vertex_extra.parseLine(csv_reader, id)) {
            vertexIds.push_back(id);
            vertexExtraValues.emplace_back(std::move(vertex_extra));
        }

        GrB_Vector id_to_index_ptr = nullptr;
        ok(LAGraph_dense_relabel(GrB_NULL, GrB_NULL, &id_to_index_ptr, vertexIds.data(), size(), GrB_NULL));
        idToIndex.reset(id_to_index_ptr);
    }
};

struct EdgeCollection {
    const BaseVertexCollection *src, *trg;
    GrB_Index edgeNumber;
    GBxx_Object<GrB_Matrix> matrix;

    EdgeCollection(const std::string &file_path,
                   const std::vector<std::reference_wrapper<BaseVertexCollection>> &vertex_collection,
                   bool transpose = false) {
        importFile(file_path, vertex_collection, transpose);
    }

    static const BaseVertexCollection &findVertexCollection(const std::string &vertex_name,
                                                            const std::vector<std::reference_wrapper<BaseVertexCollection>> &vertex_collection);

    void importFile(std::string const &file_path,
                    std::vector<std::reference_wrapper<BaseVertexCollection>> const &vertex_collection,
                    bool transpose) {
        auto[csv_file, full_column_names, header_line] = openFileWithHeader(file_path);

        char const *src_prefix = ":START_ID(", *trg_prefix = ":END_ID(", *postfix = ")";
        std::string src_column_name = full_column_names[0];
        std::string trg_column_name = full_column_names[1];
        if (transpose) {
            std::swap(src_column_name, trg_column_name);
            std::swap(src_prefix, trg_prefix);
        }
        std::string src_vertex_name = parseHeaderField(src_column_name, src_prefix, postfix);
        std::string trg_vertex_name = parseHeaderField(trg_column_name, trg_prefix, postfix);

        src = &findVertexCollection(src_vertex_name, vertex_collection);
        trg = &findVertexCollection(trg_vertex_name, vertex_collection);

        io::CSVReader<2, io::trim_chars<>, io::no_quote_escape<'|'>> csv_reader(file_path, csv_file);

        csv_reader.read_header(io::ignore_extra_column, src_column_name, trg_column_name);

        std::vector<GrB_Index> src_indices, trg_indices;
        uint64_t src_id, trg_id;
        while (csv_reader.read_row(src_id, trg_id)) {
            GrB_Index src_index, trg_index;
            ok(GrB_Vector_extractElement_UINT64(&src_index, src->idToIndex.get(), src_id));
            ok(GrB_Vector_extractElement_UINT64(&trg_index, trg->idToIndex.get(), trg_id));

            src_indices.push_back(src_index);
            trg_indices.push_back(trg_index);
        }
        edgeNumber = src_indices.size();

        matrix = GB(GrB_Matrix_new, GrB_BOOL, src->size(), trg->size());
        ok(GrB_Matrix_build_BOOL(matrix.get(),
                                 src_indices.data(), trg_indices.data(),
                                 array_of_true(edgeNumber).get(),
                                 edgeNumber, GrB_LOR));
    }
};

struct BaseQueryInput {
    std::vector<std::reference_wrapper<BaseVertexCollection>> vertexCollections;

    explicit BaseQueryInput(std::vector<std::reference_wrapper<BaseVertexCollection>> vertex_collections)
            : vertexCollections(std::move(vertex_collections)) {}
};
