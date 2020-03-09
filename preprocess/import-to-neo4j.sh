#!/bin/bash

NEO4J_CSV_DIR=$1

$NEO4J_HOME/bin/neo4j-admin import \
  --id-type=INTEGER \
  --nodes=Place="${NEO4J_CSV_DIR}/place.csv" \
  --nodes=Organisation="${NEO4J_CSV_DIR}/organisation.csv" \
  --nodes=Tag="${NEO4J_CSV_DIR}/tag.csv" \
  --nodes=Comment="${NEO4J_CSV_DIR}/comment.csv" \
  --nodes=Forum="${NEO4J_CSV_DIR}/forum.csv" \
  --nodes=Person="${NEO4J_CSV_DIR}/person.csv" \
  --relationships=IS_PART_OF="${NEO4J_CSV_DIR}/place_isPartOf_place.csv" \
  --relationships=IS_LOCATED_IN="${NEO4J_CSV_DIR}/organisation_isLocatedIn_place.csv" \
  --relationships=HAS_CREATOR="${NEO4J_CSV_DIR}/comment_hasCreator_person.csv" \
  --relationships=IS_LOCATED_IN="${NEO4J_CSV_DIR}/comment_isLocatedIn_place.csv" \
  --relationships=REPLY_OF="${NEO4J_CSV_DIR}/comment_replyOf_comment.csv" \
  --relationships=HAS_MEMBER="${NEO4J_CSV_DIR}/forum_hasMember_person.csv" \
  --relationships=HAS_TAG="${NEO4J_CSV_DIR}/forum_hasTag_tag.csv" \
  --relationships=HAS_INTEREST="${NEO4J_CSV_DIR}/person_hasInterest_tag.csv" \
  --relationships=IS_LOCATED_IN="${NEO4J_CSV_DIR}/person_isLocatedIn_place.csv" \
  --relationships=KNOWS="${NEO4J_CSV_DIR}/person_knows_person.csv" \
  --relationships=STUDY_AT="${NEO4J_CSV_DIR}/person_studyAt_organisation.csv" \
  --relationships=WORK_AT="${NEO4J_CSV_DIR}/person_workAt_organisation.csv" \
  --delimiter '|'
