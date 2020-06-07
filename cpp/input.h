#pragma once

#include "load.h"

#include <vector>

class Places : public VertexCollection<1> {
    std::vector<GrB_Index> indicesSortedByNames;

public:
    using VertexCollection::VertexCollection;

    std::vector<std::string> names;

    std::vector<std::string> extraColumns() const override {
        return {"name"};
    }

    void importFile() override {
        VertexCollection::importFile();

        sortIndicesByAttribute(names, indicesSortedByNames);
    }

    bool parseLine(CsvReaderT &csv_reader, GrB_Index &id) override {
        std::string name;
        if (csv_reader.read_row(id, name)) {
            names.push_back(std::move(name));
            return true;
        } else
            return false;
    }

    /// Find the minimum index of places having the given name
    GrB_Index findIndexByName(std::string const &name) const {
        return findIndexByAttributeValue(name, names, indicesSortedByNames);
    }
};

struct Tags : public VertexCollection<1> {
    using VertexCollection::VertexCollection;

    std::vector<std::string> names;

    std::vector<std::string> extraColumns() const override {
        return {"name"};
    }

    bool parseLine(CsvReaderT &csv_reader, GrB_Index &id) override {
        std::string name;
        if (csv_reader.read_row(id, name)) {
            names.push_back(std::move(name));
            return true;
        } else
            return false;
    }
};

struct Persons : public VertexCollection<1> {
    using VertexCollection::VertexCollection;

    std::vector<time_t> birthdays;

    std::vector<std::string> extraColumns() const override {
        return {"birthday"};
    }

    bool parseLine(CsvReaderT &csv_reader, GrB_Index &id) override {
        const char *birthday_str = nullptr;
        if (!csv_reader.read_row(id, birthday_str))
            return false;

        birthdays.push_back(parseTimestamp(birthday_str, DateFormat));

        return true;
    }
};

struct Comments : public VertexCollection<0> {
    using VertexCollection::VertexCollection;

    bool parseLine(CsvReaderT &csv_reader, GrB_Index &id) override {
        return csv_reader.read_row(id);
    }
};

struct QueryInput : public BaseQueryInput {
    Places places;
    Tags tags;
    Persons persons;
    Comments comments;

    EdgeCollection knows;
    EdgeCollection hasInterestTran;
    EdgeCollection hasCreatorTran;
    EdgeCollection hasCreator;
    EdgeCollection replyOf;

    explicit QueryInput(const BenchmarkParameters &parameters) :
            BaseQueryInput{{places, tags,            persons,        comments},
                           {knows,  hasInterestTran, hasCreatorTran, hasCreator, replyOf}},
            places{parameters.CsvPath + "place.csv"},
            tags{parameters.CsvPath + "tag.csv"},
            persons{parameters.CsvPath + "person.csv"},
            comments{parameters.CsvPath + "comment.csv"},

            knows{parameters.CsvPath + "person_knows_person.csv"},
            hasInterestTran{parameters.CsvPath + "person_hasInterest_tag.csv", true},

            hasCreatorTran{parameters.CsvPath + "comment_hasCreator_person.csv", true},
            hasCreator{parameters.CsvPath + "comment_hasCreator_person.csv"},
            replyOf{parameters.CsvPath + "comment_replyOf_comment.csv"} {
        for (auto const &collection : vertexCollections) {
            collection.get().importFile();
        }
        for (auto const &collection : edgeCollections) {
            collection.get().importFile(vertexCollections);
        }
    }

    auto comment_size() const {
        return comments.size();
    }

    auto person_size() const {
        return persons.size();
    }
};
