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

struct Comment : public Vertex {};

#pragma clang diagnostic pop

struct QueryInput : public BaseQueryInput {
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
            tags{parameters.ChangePath + "tag.csv"},
            persons{parameters.ChangePath + "person.csv"},
            comments{parameters.ChangePath + "comment.csv"},

            knows{parameters.ChangePath + "person_knows_person.csv", vertexCollections},
            hasInterestTran{parameters.ChangePath + "person_hasInterest_tag.csv", vertexCollections, true},

            hasCreatorTran{parameters.ChangePath + "comment_hasCreator_person.csv", vertexCollections, true},
            hasCreator{parameters.ChangePath + "comment_hasCreator_person.csv", vertexCollections},
            replyOf{parameters.ChangePath + "comment_replyOf_comment.csv", vertexCollections}{}

    auto comment_size() const {
        return comments.size();
    }

    auto person_size() const {
        return persons.size();
    }
};
