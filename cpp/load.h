#pragma once

#include "utils.h"
#include "csv.h"
#include "gb_utils.h"
#include <fstream>
#include <ctime>
#include <memory>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <map>
#include <utility>
#include <vector>
#include <numeric>
#include <algorithm>

std::tuple<std::ifstream, std::vector<std::string>, std::string> openFileWithHeader(const std::string &file_path);

std::string parseHeaderField(std::string const &field, char const *prefix, char const *postfix);

struct BaseVertexCollection {
    std::string vertexName;
    std::vector<GrB_Index> vertexIds;
    GBxx_Object<GrB_Vector> idToIndex;

    GrB_Index size() const {
        return vertexIds.size();
    }

    virtual void importFile() = 0;

protected:
    ~BaseVertexCollection() = default;
};

template<unsigned ExtraColumnCount>
class VertexCollection : public BaseVertexCollection {
protected:
    std::string filePath;

    using CsvReaderT = io::CSVReader<1 + ExtraColumnCount, io::trim_chars<>, io::no_quote_escape<'|'>>;

    void sortIndicesByAttribute(std::vector<std::string> const &attributes,
                                std::vector<GrB_Index> &indicesSortedByAttribute) {
        indicesSortedByAttribute.resize(size());
        std::iota(indicesSortedByAttribute.begin(), indicesSortedByAttribute.end(), 0);
        // stable sort to keep the lowest indices of duplicates first
        std::stable_sort(indicesSortedByAttribute.begin(), indicesSortedByAttribute.end(),
                         [&attributes](GrB_Index lhs, GrB_Index rhs) {
                             return attributes[lhs] < attributes[rhs];
                         });
    }

    /// Find the minimum index of vertices having the given attribute
    GrB_Index findIndexByAttributeValue(std::string const &attribute, std::vector<std::string> const &attributes,
                                        std::vector<GrB_Index> const &indicesSortedByAttribute) const {
        auto begin = indicesSortedByAttribute.begin();
        auto end = indicesSortedByAttribute.end();
        auto iter = std::lower_bound(begin, end, attribute, [&attributes](GrB_Index lhs, std::string const &rhs) {
            return attributes[lhs] < rhs;
        });

        // iter is an iterator pointing to the first element that is not less than attribute, or end if no such element is found.
        // therefore attributes[*iter] can be greater than the attribute we are looking for
        // see Possible implementation at https://en.cppreference.com/w/cpp/algorithm/binary_search
        if (iter != end && attribute == attributes[*iter])
            return *iter;
        else
            throw std::out_of_range(attribute + " is not found.");
    }

public:

    VertexCollection(std::string const &file_path) {
        filePath = file_path;
    }

    virtual std::vector<std::string> extraColumns() const {
        return {};
    }

    virtual bool parseLine(CsvReaderT &csv_reader, GrB_Index &id) = 0;

    virtual const char *getIdFieldName() const { return "id"; }

    virtual const char *getIdFieldPrefix() const { return "id:ID("; }

    void importFile() override {
        auto[csv_file, full_column_names, header_line] = openFileWithHeader(filePath);

        std::vector<std::string> selected_column_names = extraColumns();
        selected_column_names.insert(selected_column_names.begin(), getIdFieldName());

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
        vertexName = parseHeaderField(id_column, getIdFieldPrefix(), ")");

        CsvReaderT csv_reader(filePath, csv_file);

        std::array<std::string, 1 + ExtraColumnCount> selected_column_names_array;
        std::copy(selected_column_names.begin(), selected_column_names.end(), selected_column_names_array.begin());

        std::apply([&](auto... col) { csv_reader.read_header(io::ignore_extra_column, col...); },
                   selected_column_names_array);

        GrB_Index id;
        while (parseLine(csv_reader, id)) {
            vertexIds.push_back(id);
        }

        GrB_Vector id_to_index_ptr = nullptr;
        ok(LAGraph_dense_relabel(GrB_NULL, GrB_NULL, &id_to_index_ptr, vertexIds.data(), size(), GrB_NULL));
        idToIndex.reset(id_to_index_ptr);
    }
};

struct EdgeCollection {
    std::string filePath;
    bool transposed;
    const BaseVertexCollection *src, *trg;
    GrB_Index edgeNumber;
    GBxx_Object<GrB_Matrix> matrix;

    EdgeCollection(const std::string &file_path, bool transposed = false)
            : filePath(file_path), transposed(transposed) {}

    static const BaseVertexCollection &findVertexCollection(const std::string &vertex_name,
                                                            const std::vector<std::reference_wrapper<BaseVertexCollection>> &vertex_collection);

    virtual void importFile(std::vector<std::reference_wrapper<BaseVertexCollection>> const &vertex_collection) {
        auto[csv_file, full_column_names, header_line] = openFileWithHeader(filePath);

        char const *src_prefix = ":START_ID(", *trg_prefix = ":END_ID(", *postfix = ")";
        std::string src_column_name = full_column_names[0];
        std::string trg_column_name = full_column_names[1];
        if (transposed) {
            std::swap(src_column_name, trg_column_name);
            std::swap(src_prefix, trg_prefix);
        }
        std::string src_vertex_name = parseHeaderField(src_column_name, src_prefix, postfix);
        std::string trg_vertex_name = parseHeaderField(trg_column_name, trg_prefix, postfix);

        src = &findVertexCollection(src_vertex_name, vertex_collection);
        trg = &findVertexCollection(trg_vertex_name, vertex_collection);

        io::CSVReader<2, io::trim_chars<>, io::no_quote_escape<'|'>> csv_reader(filePath, csv_file);

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

struct TransposedEdgeCollection : public EdgeCollection {
    EdgeCollection const &baseEdge;

    TransposedEdgeCollection(const EdgeCollection &base_edge)
            : EdgeCollection(base_edge.filePath, true), baseEdge(base_edge) {}

    void importFile(const std::vector<std::reference_wrapper<BaseVertexCollection>> &vertex_collection) override {
        src = baseEdge.trg;
        trg = baseEdge.src;
        edgeNumber = baseEdge.edgeNumber;

        matrix = GB(GrB_Matrix_new, GrB_BOOL, src->size(), trg->size());
        ok(GrB_transpose(matrix.get(), GrB_NULL, GrB_NULL, baseEdge.matrix.get(), GrB_NULL));
    }
};

struct BaseQueryInput {
    std::vector<std::reference_wrapper<BaseVertexCollection>> vertexCollections;
    std::vector<std::reference_wrapper<EdgeCollection>> edgeCollections;
};
