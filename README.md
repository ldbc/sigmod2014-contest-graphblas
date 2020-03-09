# SIGMOD 2014 Programming Contest

## Loading the data

Navigate to the `preprocess/` directory and use the `convert-csvs.sh` script with the corresponding character encoding.

```bash
cd preprocess/
./convert-csvs.sh outputDir-1k outputDir-1k-converted macintosh
./convert-csvs.sh outputDir-10k outputDir-10k-converted
```

Then load the data from the converted CSVs to Neo4j. Set the `NEO4J_HOME` environment variable and invoke the shell script provided.

```bash
export NEO4J_HOME=
export CSV_INPUT_DIR=
./delete-neo4j-database.sh
./import-to-neo4j.sh $CSV_INPUT_DIR
$NEO4J_HOME/bin/neo4j-admin set-initial-password admin
./restart-neo4j.sh
```

Once Neo4j is running, open the web browswer and delete redundant `KNOWS` edges:

```
MATCH (p1:Person)-[k:KNOWS]->(p2:Person)
WHERE p1.id < p2.id
DELETE k
```

## Queries

### Q1

Cleanup:
```
MATCH ()-[f:FREQ_COMM]->()
DELETE f
```

Construct overlay graph:
```
:param [{threshold, p1id, p2id}] => {RETURN 4 AS threshold, 848 AS p1id, 414 AS p2id}
```
```
MATCH (p1:Person)-[:KNOWS]-(p2:Person),
  (c1:Comment)-[:HAS_CREATOR]->(p1),
  (c2:Comment)-[:HAS_CREATOR]->(p2),
  (c1)-[r:REPLY_OF]-(c2)
WITH p1, p2, collect({c1: c1, r: r, c2: c2}) AS interactions
WITH p1, p2, [cs IN interactions WHERE startNode(cs.r) = cs.c1 | cs] AS fwd, [cs IN interactions WHERE endNode(cs.r) = cs.c1 | cs] AS bwd
WHERE size(fwd) > $threshold AND size(bwd) > $threshold
CREATE (p1)-[:FREQ_COMM]->(p2)

```

Shortest path:
```
MATCH s=shortestPath((p1:Person {id: $p1id})-[:FREQ_COMM*]-(p2:Person {id: $p2id}))
RETURN p1.id, p2.id, s
```
```
MATCH s=shortestPath((p1:Person {id: $p1id})-[:FREQ_COMM*]-(p2:Person {id: $p2id}))
RETURN p1.id, p2.id, [n IN nodes(s) | n.id]
```
