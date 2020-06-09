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

    GrB_Index findIndexByName(std::string const &name) const {
        return findIndexByAttributeValue(name, names, indicesSortedByNames);
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

struct Comments : public VertexCollection<1> {
    using VertexCollection::VertexCollection;
    /// loaded as IDs, later transformed to indices when hasCreator edge is loaded
    std::vector<GrB_Index> creatorPersonIndices;

    std::vector<std::string> extraColumns() const override {
        return {":END_ID(Person)"};
    }

    const char *getIdFieldName() const override {
        return ":START_ID(Comment)";
    }

    const char *getIdFieldPrefix() const override {
        return ":START_ID(";
    }

    bool parseLine(CsvReaderT &csv_reader, GrB_Index &id) override {
        GrB_Index creator_person_id;
        if (csv_reader.read_row(id, creator_person_id)) {
            creatorPersonIndices.push_back(creator_person_id);
            return true;
        } else
            return false;
    }
};

struct HasCreatorEdgeCollection : public EdgeCollection {
    Comments &comments;
    Persons const &persons;

    HasCreatorEdgeCollection(Comments &comments, Persons const &persons)
            : EdgeCollection("", false), comments(comments), persons(persons) {}

    void importFile(const std::vector<std::reference_wrapper<BaseVertexCollection>> &vertex_collection) override {
        src = &comments;
        trg = &persons;
        edgeNumber = comments.size();

        // indices of comments
        std::vector<GrB_Index> comment_indices;
        comment_indices.resize(comments.size());
        std::iota(comment_indices.begin(), comment_indices.end(), 0);

        // convert person IDs to indices in comments
        for (int comment_index = 0; comment_index < comments.size(); ++comment_index) {
            ok(GrB_Vector_extractElement_UINT64(&comments.creatorPersonIndices[comment_index], persons.idToIndex.get(),
                                                comments.creatorPersonIndices[comment_index]));
        }

        matrix = GB(GrB_Matrix_new, GrB_BOOL, src->size(), trg->size());
        ok(GrB_Matrix_build_BOOL(matrix.get(),
                                 comment_indices.data(), comments.creatorPersonIndices.data(),
                                 array_of_true(edgeNumber).get(),
                                 edgeNumber, GrB_LOR));
    }
};

struct Forums : public VertexCollection<0> {
    using VertexCollection::VertexCollection;

    bool parseLine(CsvReaderT &csv_reader, GrB_Index &id) override {
        return csv_reader.read_row(id);
    }
};

struct QueryInput : public BaseQueryInput {
    Places places;
    Tags tags;
    Forums forums;
    Persons persons;
    Comments comments;

    EdgeCollection knows;
    EdgeCollection hasInterestTran;
    HasCreatorEdgeCollection hasCreator;
    TransposedEdgeCollection hasCreatorTran;
    EdgeCollection replyOf;
    EdgeCollection hasTag;
    EdgeCollection hasMember;

    explicit QueryInput(const BenchmarkParameters &parameters) :
            places{parameters.CsvPath + "place.csv"},
            tags{parameters.CsvPath + "tag.csv"},
            forums{parameters.CsvPath + "forum.csv"},
            persons{parameters.CsvPath + "person.csv"},
            comments{parameters.CsvPath + "comment_hasCreator_person.csv"},

            knows{parameters.CsvPath + "person_knows_person.csv"},
            hasInterestTran{parameters.CsvPath + "person_hasInterest_tag.csv", true},
            hasCreator{comments, persons},
            hasCreatorTran{hasCreator},
            replyOf{parameters.CsvPath + "comment_replyOf_comment.csv"},
            hasTag{parameters.CsvPath + "forum_hasTag_tag.csv"},
            hasMember{parameters.CsvPath + "forum_hasMember_person.csv"} {
        switch (parameters.Query) {
            case 1:
                vertexCollections = {comments, persons};
                edgeCollections = {knows, hasCreator, hasCreatorTran, replyOf};
                break;
            case 2:
                vertexCollections = {tags, persons};
                edgeCollections = {knows, hasInterestTran};
                break;
            case 3:
                vertexCollections = {places, tags, persons};
                edgeCollections = {knows, hasInterestTran};
                break;
            case 4:
                vertexCollections = {tags, forums, persons};
                edgeCollections = {knows, hasTag, hasMember};
                break;
            default:
                vertexCollections = {places, tags, forums, persons, comments};
                edgeCollections = {knows, hasInterestTran, hasCreator, hasCreatorTran, replyOf, hasTag, hasMember};
                break;
        }

        for (auto const &collection : vertexCollections) {
            collection.get().importFile();
        }
        for (auto const &collection : edgeCollections) {
            collection.get().importFile(vertexCollections);
        }
    }
};
