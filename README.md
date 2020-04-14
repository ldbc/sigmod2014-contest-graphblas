# SIGMOD 2014 Programming Contest

See our website at <https://ftsrg.mit.bme.hu/paper-lsgda2020/>.

## Use [pygraphblas](https://github.com/michelp/pygraphblas) in notebooks

### Requirements
- Docker

### Steps
- Clone [fixes branch](https://github.com/szarnyasg/pygraphblas/tree/fixes) of https://github.com/szarnyasg/pygraphblas/
- Run `./build.sh v3.2.0` to build the Docker image with slight modifications
- Mount this repo to the container by adding `-v «PATH»/sigmod2014-pc:/home/jovyan/sigmod2014-pc` to [notebook.sh](https://github.com/szarnyasg/pygraphblas/blob/fixes/notebook.sh)
- Start `./notebook.sh`
- Open Jupyter Notebook at: http://127.0.0.1:8888/?token=«TOKEN»

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
export NEO4J_DATA_DIR=${NEO4J_HOME}/data/
export CSV_INPUT_DIR=
echo -e "$NEO4J_HOME\n$NEO4J_DATA_DIR\n$CSV_INPUT_DIR"
./delete-neo4j-database.sh
./import-to-neo4j.sh
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

Set the parameters:
```
:param [{p1id, p2id, threshold}] => {
  WITH [858, 587, 1] AS params
  RETURN params[0] AS p1id, params[1] AS p2id, params[2] AS threshold
}
```

#### Approach #1

Cleanup, construct the overlay graph, query and compute the shortest path using a single query:
```
MATCH ()-[f:FREQ_COMM]->()
DELETE f

WITH count(*) AS dummy

MATCH
  (p1:Person)-[:KNOWS]-(p2:Person)

// p1 has written enough replies
OPTIONAL MATCH
  (c1:Comment)-[:HAS_CREATOR]->(p1),
  (c2:Comment)-[:HAS_CREATOR]->(p2),
  (c1)-[r:REPLY_OF]->(c2)
WITH p1, p2, count(DISTINCT c1) AS c1count
WHERE c1count > $threshold

// p2 has written enough replies
OPTIONAL MATCH
  (c1:Comment)-[:HAS_CREATOR]->(p1),
  (c2:Comment)-[:HAS_CREATOR]->(p2),
  (c1)<-[r:REPLY_OF]-(c2)
WITH p1, p2, count(DISTINCT c2) AS c2count
WHERE c2count > $threshold

CREATE (p1)-[:FREQ_COMM]->(p2)

WITH count(*) AS dummy

MATCH s=shortestPath((p1:Person {id: $p1id})-[:FREQ_COMM*]-(p2:Person {id: $p2id}))
RETURN p1.id, p2.id, [n IN nodes(s) | n.id]
```

This query takes approx 1.5-2 minutes.

#### Approach #2

Precompute the number of interactions:
```
MATCH (p1:Person)-[k:KNOWS]-(p2:Person)
WHERE p1.id < p2.id

// p1's replies
OPTIONAL MATCH
  (c1:Comment)-[:HAS_CREATOR]->(p1),
  (c2:Comment)-[:HAS_CREATOR]->(p2),
  (c1)-[r:REPLY_OF]->(c2)
WITH p1, p2, k, count(DISTINCT c1) AS c1count

// p2's replies
OPTIONAL MATCH
  (c1:Comment)-[:HAS_CREATOR]->(p1),
  (c2:Comment)-[:HAS_CREATOR]->(p2),
  (c1)<-[r:REPLY_OF]-(c2)
WITH k, c1count, count(DISTINCT c2) AS c2count

// we use CASE to express min(c1count, c2count)
SET k.interactions = CASE WHEN c1count < c2count THEN c1count ELSE c2count END
```

Run the query as follows:
```
MATCH ()-[f:FREQ_COMM]->()
DELETE f

WITH count(*) AS dummy

// add a FREQ_COMM edge where the number of interactions is above the threshold
MATCH (p1:Person)-[k:KNOWS]->(p2:Person)
WHERE k.interactions > $threshold
CREATE (p1)-[:FREQ_COMM]->(p2)

WITH count(*) AS dummy

MATCH s=shortestPath((p1:Person {id: $p1id})-[:FREQ_COMM*]-(p2:Person {id: $p2id}))
RETURN p1.id, p2.id, [n IN nodes(s) | n.id]
```


### Q2

Install [APOC 4.0.0.6](https://github.com/neo4j-contrib/neo4j-apoc-procedures/releases/download/4.0.0.6/apoc-4.0.0.6-all.jar) to `$NEO4J_HOME/plugins`.
Edit the `$NEO4J_HOME/conf/neo4j.conf` file, add the following lines and restart the database:
```
dbms.security.procedures.unrestricted=apoc.*
dbms.security.procedures.whitelist=apoc.*
```

:warning: The queries are destructive. Load the database before new query.

Set parameter:
```
:param [{topK, birthday}] => {
  WITH [3, '1986-06-14'] AS params
  RETURN params[0] AS topK, date(params[1]) AS birthday
}
```
Expected result: `Chiang_Kai-shek Mohandas_Karamchand_Gandhi Joseph_Stalin % component sizes 6 6 5`

```
MATCH (t:Tag)<-[:HAS_INTEREST]-(p:Person)
WHERE p.birthday>=$birthday
WITH t, collect(p) AS persons
CALL apoc.create.addLabels(persons, ['HasInterest_' + t.id]) YIELD node
RETURN count(*)
```

```
CALL apoc.periodic.commit("
  MATCH (t:Tag)<-[:HAS_INTEREST]-(p1:Person)
  WHERE p1.birthday>=$birthday
  WITH t, min(p1) AS p1
  CREATE (t)-[:COMPONENT]->(comp:Component)
  WITH t, p1, comp
  CALL apoc.path.subgraphNodes(p1,
    {labelFilter: 'HasInterest_' + t.id,
    relationshipFilter: 'KNOWS'}) YIELD node AS p2
  CREATE (comp)-[:PERSON]->(p2)
  WITH t, comp, p2
  MATCH (t)<-[hi:HAS_INTEREST]-(p2)
  DELETE hi
  WITH t, comp, count(*) AS componentSize
  SET comp.size = componentSize
  RETURN count(*)
  // limit - to bypass mandatory limit",
  {birthday: $birthday})
```

```
MATCH (t:Tag)
OPTIONAL MATCH (t)-[:COMPONENT]->(comp:Component)
return t.name, coalesce(max(comp.size), 0) AS score
ORDER BY score DESC, t.name ASC
LIMIT $topK
```

### Q3

Set the `$place` parameter, e.g.

```
:param place => "South_America"
```

```
// set parameters k and h manually:
// - k in the last `LIMIT` clause
// - h in the `KNOWS*1...` bit of the last MATCH clause
CYPHER runtime=interpreted
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
UNWIND persXnodes AS p1
MATCH
  (p1)-[:HAS_INTEREST]->(t:Tag)<-[:HAS_INTEREST]-(p2:Person),
  (p1)-[:KNOWS*1..2]-(p2)
WHERE p1.id < p2.id
  AND p1 IN persXnodes
  AND p2 IN persXnodes
WITH p1, p2, count(DISTINCT t) AS numTags
RETURN p1.id, p2.id, numTags
ORDER BY numTags DESC, p1.id ASC, p2.id ASC
LIMIT 3
```

Note that in Neo4j Enterprise, you need to use `CYPHER runtime=interpreted` due to a [bug in the slotted runtime](https://github.com/neo4j/neo4j/issues/12441).

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

#### Prerequisites

Edit the `$NEO4J_HOME/conf/neo4j.conf` file, add the following line and restart the database:
```
cypher.forbid_shortestpath_common_nodes=false
```

Cleanup between experiments by running:
```
MATCH ()-[f:MEMBERFRIENDS]->()
DELETE f
```

#### Queries

Set tag parameter `$t`:
```
:param t => 'Bill_Clinton'
```

Construct the overlay graph:
```
MATCH
  (t:Tag {name: $t})<-[:HAS_TAG]-(f1:Forum)
MATCH
  (t:Tag {name: $t})<-[:HAS_TAG]-(f2:Forum)
MATCH
  (f1)-[:HAS_MEMBER]->(personA),
  (f2)-[:HAS_MEMBER]->(personB),
  (personA:Person)-[:KNOWS]-(personB:Person)
WHERE ID(personA) < ID(personB)
WITH DISTINCT personA, personB
CREATE (personA)-[:MEMBERFRIENDS]->(personB)
```

```
// n
MATCH (anyPerson:Person)
WHERE (anyPerson)-[:MEMBERFRIENDS]-()
WITH count(anyPerson) AS n

// optimization
UNWIND [385, 492, 819] AS pID MATCH (p:Person {id: pID})
// s: the sum of geodesic distances to all other reachable persons from p
// r: number of vertices reachable from p (inclusive)
MATCH path=shortestPath((p)-[:MEMBERFRIENDS*0..]-(p2:Person))
WITH p, n, count(DISTINCT p2) AS r, sum(length(path)) AS s
WHERE s <> 0
RETURN p.id, ((r-1)*(r-1))/toFloat((n-1)*s) AS score
ORDER BY score DESC
LIMIT 3
```
