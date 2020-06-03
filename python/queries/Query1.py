from collections import namedtuple

from queries.QueryBase import QueryBase
from algorithms.search import naive_bfs_levels
from pygraphblas import *
#from _pygraphblas import lib

Test = namedtuple('Test', ['inputs', 'expected_result'])


class Query1(QueryBase):

    def __init__(self, data_dir, data_format, params):
        self.tests = self.init_tests()
        super().__init__(data_dir, data_format, self.tests)
        self.person1_id = params[0]
        self.person2_id = params[1]
        self.num_of_interactions = params[2]
        self.person = None
        self.comment = None
        self.replyOf = None
        self.knows = None
        self.hasCreator = None

    def execute_query(self, params=None, search_method=naive_bfs_levels):
        if params is not None:
            self.person1_id = params[0]
            self.person2_id = params[1]
            self.num_of_interactions = params[2]

        if self.person is None:
            # Load vertices and edges
            self.person = self.loader.load_vertex('person')
            self.comment = self.loader.load_vertex('comment')
            self.replyOf = self.loader.load_edge('replyOf', self.comment, self.comment)
            self.knows = self.loader.load_edge('knows', self.person, self.person)
            self.hasCreator = self.loader.load_edge('hasCreator', self.comment, self.person)

        # Run query
        person1_id_remapped = self.person.index2id[self.person1_id]
        person2_id_remapped = self.person.index2id[self.person2_id]
        if self.num_of_interactions == -1:
            overlay_graph = self.knows
        else:
            hasCreatorTransposed =self.hasCreator.transpose()
            personA_to_comment2 = hasCreatorTransposed @ self.replyOf
            person_to_person = personA_to_comment2.mxm(self.hasCreator, mask=self.knows)
            person_to_person_filtered = person_to_person.select(lib.GxB_GT_THUNK, self.num_of_interactions)
            overlay_graph = person_to_person_filtered.pattern()

        levels = search_method(overlay_graph, person1_id_remapped)
        try:
            result = levels[person2_id_remapped] - 1  # Get hop count
        except:
            # There is no path
            result = -1

        return result

    def format_result_string(self, result):
        return int(result.split('%')[0])

    def init_tests(self):
        tests = [
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
