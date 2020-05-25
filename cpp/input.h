#pragma once

#include "load.h"

struct Vertex {
    static auto extraColumns() {
        return array_of<char const *>();
    }

    template<typename Reader>
    bool parseLine(Reader &csv_reader, GrB_Index &id) {
        return csv_reader.read_row(id);
    }
};

#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"

struct Place : public Vertex {
    std::string name;

    static auto extraColumns() {
        return array_of<char const *>("name");
    }

    template<typename Reader>
    bool parseLine(Reader &csv_reader, GrB_Index &id) {
        return csv_reader.read_row(id, name);
    }
};

struct Tag : public Vertex {
    std::string name;

    static auto extraColumns() {
        return array_of<char const *>("name");
    }

    template<typename Reader>
    bool parseLine(Reader &csv_reader, GrB_Index &id) {
        return csv_reader.read_row(id, name);
    }
};

struct Person : public Vertex {
    time_t birthday;

    static auto extraColumns() {
        return array_of<char const *>("birthday");
    }

    template<typename Reader>
    bool parseLine(Reader &csv_reader, GrB_Index &id) {
        const char *birthday_str = nullptr;
        if (!csv_reader.read_row(id, birthday_str))
            return false;

        birthday = parseTimestamp(birthday_str, DateFormat);

        return true;
    }
};

struct Comment : public Vertex {};

#pragma clang diagnostic pop

struct QueryInput : public BaseQueryInput {
    VertexCollection<Place> places;
    VertexCollection<Tag> tags;
    VertexCollection<Person> persons;
    VertexCollection<Comment> comments;

    EdgeCollection knows;
    EdgeCollection hasInterestTran;
    EdgeCollection hasCreatorTran;
    EdgeCollection hasCreator;
    EdgeCollection replyOf;

    explicit QueryInput(const BenchmarkParameters &parameters) :
            BaseQueryInput{{tags, persons, comments}},
            places{parameters.CsvPath + "place.csv"},
            tags{parameters.CsvPath + "tag.csv"},
            persons{parameters.CsvPath + "person.csv"},
            comments{parameters.CsvPath + "comment.csv"},

            knows{parameters.CsvPath + "person_knows_person.csv", vertexCollections},
            hasInterestTran{parameters.CsvPath + "person_hasInterest_tag.csv", vertexCollections, true},

            hasCreatorTran{parameters.CsvPath + "comment_hasCreator_person.csv", vertexCollections, true},
            hasCreator{parameters.CsvPath + "comment_hasCreator_person.csv", vertexCollections},
            replyOf{parameters.CsvPath + "comment_replyOf_comment.csv", vertexCollections}{}

    auto comment_size() const {
        return comments.size();
    }

    auto person_size() const {
        return persons.size();
    }
};
