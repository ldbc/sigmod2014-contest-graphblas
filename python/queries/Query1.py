from collections import namedtuple
from timeit import default_timer as timer
import logging

from queries.QueryBase import QueryBase
from algorithms.search import naive_bfs_levels, push_pull_bfs_levels, msbfs_levels, push_pull_msbfs_levels
from pygraphblas import *

Test = namedtuple('Test', ['inputs', 'expected_result'])

handler = logging.StreamHandler()
handler.setFormatter(logging.Formatter('%(asctime)s %(levelname)-5s %(message)s'))
log = logging.getLogger(__name__)
log.propagate = False
log.addHandler(handler)
log.setLevel(logging.INFO)


class Query1(QueryBase):

    def __init__(self, data_dir, data_format):
        self.tests = self.init_tests()
        super().__init__(data_dir, data_format, self.tests)
        self.person1_id = None
        self.person2_id = None
        self.num_of_interactions = None
        self.person = None
        self.comment = None
        self.replyOf = None
        self.knows = None
        self.hasCreator = None

    def load_data(self):
        load_start = timer()
        self.person = self.loader.load_vertex('person')
        self.comment = self.loader.load_vertex('comment')
        self.replyOf = self.loader.load_edge('replyOf', self.comment, self.comment)
        self.knows = self.loader.load_edge('knows', self.person, self.person)
        self.hasCreator = self.loader.load_edge('hasCreator', self.comment, self.person)
        load_end = timer()
        self.load_time = load_end - load_start
        log.info(f'Loading took: {self.load_time} seconds')

    def execute_query(self, params, search_method=push_pull_bfs_levels):

        self.person1_id = params[0]
        self.person2_id = params[1]
        self.num_of_interactions = params[2]

        if self.person is None:
            self.load_data()

        # Run query
        query_start = timer()
        person1_id_remapped = self.person.id2index[self.person1_id]
        person2_id_remapped = self.person.id2index[self.person2_id]
        if self.num_of_interactions == -1:
            overlay_graph = self.knows
        else:
            # pattern: a hacky way to cast to UINT64 for next steps, where count is required instead of existence
            hasCreatorTransposed =self.hasCreator.transpose().pattern(UINT64)
            personA_to_comment2 = hasCreatorTransposed @ self.replyOf
            person_to_person = personA_to_comment2.mxm(self.hasCreator, mask=self.knows)
            person_to_person_filtered = person_to_person.select(lib.GxB_GT_THUNK, self.num_of_interactions)

            # make symmetric by removing one-directional freqComm relationships
            person_to_person_filtered.emult(person_to_person_filtered,
                                            mult_op=binaryop.PAIR_UINT64,
                                            desc=lib.GrB_DESC_T0,
                                            out=person_to_person_filtered)

            overlay_graph = person_to_person_filtered.pattern()

        levels = search_method(overlay_graph, person1_id_remapped)
        try:
            result = levels[person2_id_remapped] - 1  # Get hop count
        except:
            # There is no path
            result = -1

        query_end = timer()
        self.test_execution_times.append(query_end - query_start)
        log.info(f'Query took: {query_end - query_start} second')
        return result

    def format_result_string(self, result):
        return int(result.split('%')[0])

    def init_tests(self):
        tests = [
            Test([786, 799, 1], '4 % path 786-63-31-60-799 (other shortest paths may exist)'),
            Test([422, 736, 1], '-1 % path none'),
            Test([576, 400, -1], '3 % path 576-618-951-400 (other shortest paths may exist)'),
            Test([58, 402, 0], '3 % path 58-935-808-402 (other shortest paths may exist)'),
            Test([266, 106, -1], '3 % path 266-23-592-106 (other shortest paths may exist)'),
            Test([313, 523, -1], '-1 % path none'),
            Test([858, 587, 1], '4 % path 858-46-31-162-587 (other shortest paths may exist)'),
            Test([155, 355, -1], '3 % path 155-21-0-355 (other shortest paths may exist)'),
            Test([947, 771, -1], '2 % path 947-625-771 (other shortest paths may exist)'),
            Test([105, 608, 3], '-1 % path none'),
            Test([128, 751, -1], '3 % path 128-459-76-751 (other shortest paths may exist)'),
            Test([814, 641, 0], '3 % path 814-109-557-641 (other shortest paths may exist)')
        ]
        return tests
