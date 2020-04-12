#!/bin/bash

${NEO4J_HOME}/bin/neo4j-admin import \
  --id-type=INTEGER \
  --nodes=Place="${CSV_INPUT_DIR}/place.csv" \
  --nodes=Organisation="${CSV_INPUT_DIR}/organisation.csv" \
  --nodes=Tag="${CSV_INPUT_DIR}/tag.csv" \
  --nodes=Comment="${CSV_INPUT_DIR}/comment.csv" \
  --nodes=Forum="${CSV_INPUT_DIR}/forum.csv" \
  --nodes=Person="${CSV_INPUT_DIR}/person.csv" \
  --relationships=IS_PART_OF="${CSV_INPUT_DIR}/place_isPartOf_place.csv" \
  --relationships=IS_LOCATED_IN="${CSV_INPUT_DIR}/organisation_isLocatedIn_place.csv" \
  --relationships=HAS_CREATOR="${CSV_INPUT_DIR}/comment_hasCreator_person.csv" \
  --relationships=IS_LOCATED_IN="${CSV_INPUT_DIR}/comment_isLocatedIn_place.csv" \
  --relationships=REPLY_OF="${CSV_INPUT_DIR}/comment_replyOf_comment.csv" \
  --relationships=HAS_MEMBER="${CSV_INPUT_DIR}/forum_hasMember_person.csv" \
  --relationships=HAS_TAG="${CSV_INPUT_DIR}/forum_hasTag_tag.csv" \
  --relationships=HAS_INTEREST="${CSV_INPUT_DIR}/person_hasInterest_tag.csv" \
  --relationships=IS_LOCATED_IN="${CSV_INPUT_DIR}/person_isLocatedIn_place.csv" \
  --relationships=KNOWS="${CSV_INPUT_DIR}/person_knows_person.csv" \
  --relationships=STUDY_AT="${CSV_INPUT_DIR}/person_studyAt_organisation.csv" \
  --relationships=WORK_AT="${CSV_INPUT_DIR}/person_workAt_organisation.csv" \
  --delimiter '|'
