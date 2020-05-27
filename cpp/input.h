#pragma once

#include "load.h"

#include <vector>
#include <algorithm>
#include <numeric>

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

        indicesSortedByNames.resize(size());
        std::iota(indicesSortedByNames.begin(), indicesSortedByNames.end(), 0);
        // stable sort to keep the lowest indices of duplicates first
        std::stable_sort(indicesSortedByNames.begin(), indicesSortedByNames.end(),
                         [this](GrB_Index lhs, GrB_Index rhs) {
                             return names[lhs] < names[rhs];
                         });
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
        auto begin = indicesSortedByNames.begin();
        auto end = indicesSortedByNames.end();
        auto iter = std::lower_bound(begin, end, name, [this](GrB_Index lhs, std::string const &rhs) {
            return names[lhs] < rhs;
        });

        // iter is an iterator pointing to the first element that is not less than name, or end if no such element is found.
        // therefore names[*iter] can be greater than the name we are looking for
        // see Possible implementation at https://en.cppreference.com/w/cpp/algorithm/binary_search
        if (iter != end && name == names[*iter])
            return *iter;
        else
            throw std::out_of_range(name + " is not found.");
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
