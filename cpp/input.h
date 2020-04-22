#pragma once

#include "load.h"

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

struct QueryInput : public BaseQueryInput {
    VertexCollection<Tag> tags;
    VertexCollection<Person> persons;

    EdgeCollection knows;
    EdgeCollection hasInterestTran;

    explicit QueryInput(const BenchmarkParameters &parameters) :
            BaseQueryInput{{tags, persons}},
            tags{parameters.ChangePath + "tag.csv"},
            persons{parameters.ChangePath + "person.csv"},
            knows{parameters.ChangePath + "person_knows_person.csv", vertexCollections},
            hasInterestTran{parameters.ChangePath + "person_hasInterest_tag.csv", vertexCollections, true} {}
};
