# SIGMOD 2014 Programming Contest

## Data

For quick experiments on graphs (e.g. running Cypher queries to see the expected result of an algorithm), we use the [Neo4j 4.0 Community](https://neo4j.com/download-center/#community) graph database.

### Preprocessing

Navigate to the `preprocess/` directory and use the `convert-csvs.sh` script with the files and their respective character encoding.

```bash
cd preprocess/
./convert-csvs.sh /path/to/outputDir-1k /path/to/outputDir-1k-converted macintosh
./convert-csvs.sh /path/to/outputDir-10k /path/to/outputDir-10k-converted
```

:bulb: It is possible to skip this step – data sets that **have already been converted** with this script are available in Gabor's Dropbox as zip archives:

* 1k persons: <https://www.dropbox.com/s/sgrwihjji551teq/sf1k.zip?dl=1>
* 10k persons: <https://www.dropbox.com/s/goabh7c3q5k4ex4/sf10k.zip?dl=1>

### Loading

:warning: These scripts are potentially destructive. Be cautious when running them on your machine.

To load the data from the converted CSVs to Neo4j, set the `NEO4J_HOME` and the `CSV_INPUT_DIR` environment variables, and invoke the provided shell scripts as follows:

```bash
export NEO4J_HOME=
export CSV_INPUT_DIR=
./delete-neo4j-database.sh
./import-to-neo4j.sh $CSV_INPUT_DIR
$NEO4J_HOME/bin/neo4j-admin set-initial-password admin
./restart-neo4j.sh
```

Once Neo4j is running, open the web browser and delete the redundant `KNOWS` edges (thus effective converting the `KNOWS` graph to directed which is the recommended way of storing undirected edges in a graph database):

```
MATCH (p1:Person)-[k:KNOWS]->(p2:Person)
WHERE p1.id < p2.id
DELETE k
```

Now all is set to work with the queries in the Programming Contest.

## Queries

### Q1

Cleanup step between runs:
```
MATCH ()-[f:FREQ_COMM]->()
DELETE f
```

Constructing the overlay graph:
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

Compute the Shortest path:
```
MATCH s=shortestPath((p1:Person {id: $p1id})-[:FREQ_COMM*]-(p2:Person {id: $p2id}))
RETURN p1.id, p2.id, s
```
To get *just the node ids* in the path, you can use a list comprehension.
```
MATCH s=shortestPath((p1:Person {id: $p1id})-[:FREQ_COMM*]-(p2:Person {id: $p2id}))
RETURN p1.id, p2.id, [n IN nodes(s) | n.id]
```

### Q2

TBD.

### Q3

Set the `$place` parameter, e.g.

```
:param place => "South_America"
```

```
// set parameters k and h manually:
// - k in the last `LIMIT` clause
// - h in the `KNOWS*1...` bit of the last MATCH clause
CALL {
    MATCH (persX:Person)-[:IS_LOCATED_IN]->(:Place)-[:IS_PART_OF*0..2]->(:Place {name: $place})
    RETURN persX
  UNION
    MATCH (persX:Person)-[:WORK_AT]->(:Company)-[:IS_LOCATED_IN]->(country:Place)-[:IS_PART_OF*0..1]->(:Place {name: $place})
    RETURN persX
  UNION
    MATCH (persX:Person)-[:STUDY_AT]->(:University)-[:IS_LOCATED_IN]->(city:Place)-[:IS_PART_OF*0..2]->(:Place {name: $place})
    RETURN persX
}
// option 1: return nodes with
// RETURN persX
// option 2: label nodes with
// SET persX:PersX
// option 3: compute the scores using the following (inefficient) method:
WITH collect(persX) AS persXnodes
MATCH
  (p1:Person)-[:HAS_INTEREST]->(t:Tag)<-[:HAS_INTEREST]-(p2:Person),
  (p1)-[:KNOWS*1..2]-(p2)
WHERE p1.id < p2.id
  AND p1 IN persXnodes
  AND p2 IN persXnodes
WITH p1, p2, count(DISTINCT t) AS numTags
RETURN p1.id, p2.id, numTags
ORDER BY numTags DESC, p1.id ASC, p2.id ASC
LIMIT 3
```

If you go with option 2, you can compute the results with a separate query:

```
// set parameters k and h manually:
// - k in the last `LIMIT` clause
// - h in the `KNOWS*1...` expression
MATCH
  (p1:PersX)-[:HAS_INTEREST]->(t:Tag)<-[:HAS_INTEREST]-(p2:PersX),
  (p1)-[:KNOWS*1..2]-(p2)
WHERE p1.id < p2.id
WITH p1, p2, count(DISTINCT t) AS numTags
RETURN p1.id, p2.id, numTags
ORDER BY numTags DESC, p1.id ASC, p2.id ASC
LIMIT 3
```

Cleanup labels:

```
MATCH (p:PersX)
REMOVE p:PersX
```

### Q4

TBD.
