#include <ctime>
#include <memory>
#include <iostream>
#include "load.h"
#include <cassert>
#include "csv.h"

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
                                     return vertex.vertexName == vertex_name;
                                 });

    if (iterator == vertex_collection.end())
        throw std::invalid_argument{"Vertex is not loaded: " + vertex_name};

    return *iterator;
}
