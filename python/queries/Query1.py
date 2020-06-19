from collections import namedtuple
from timeit import default_timer as timer
import logging

from queries.QueryBase import QueryBase
from algorithms.search import naive_bfs_levels, push_pull_bfs_levels, msbfs_levels, push_pull_msbfs_levels, bidirectional_bfs
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
        self.person, _ = self.loader.load_vertex('person')
        self.comment, _ = self.loader.load_vertex('comment')
        self.replyOf = self.loader.load_edge('replyOf', self.comment, self.comment)
        self.knows = self.loader.load_edge('knows', self.person, self.person)
        self.hasCreator = self.loader.load_edge('hasCreator', self.comment, self.person)
        load_end = timer()
        self.load_time = load_end - load_start
        #log.info(f'Loading took: {self.load_time} seconds')

    def execute_query(self, params, search_method=bidirectional_bfs):

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

        if search_method == bidirectional_bfs:
            # Experimenting with bidir bfs here
            numpersons = len(self.person.id2index)
            frontier1 = Vector.from_lists([person1_id_remapped], [True], numpersons)
            frontier2 = Vector.from_lists([person2_id_remapped], [True], numpersons)
            result = search_method(overlay_graph, frontier1, frontier2)

        else:
            levels = search_method(overlay_graph, person1_id_remapped)
            try:
                result = levels[person2_id_remapped] - 1  # Get hop count
            except:
                # There is no path
                result = -1

        query_end = timer()
        self.test_execution_times.append(query_end - query_start)
        # log.info(f'Query took: {query_end - query_start} second')
        print(f'q1,{int(self.load_time*10**6)},{int((query_end - query_start)*10**6)},{result}')
        # log.info(result)
        return result

    # Optimized version: do not create overlay graph but investigate investigate KNOWS edges on-the-fly.
    def step_frontier(self, frontier, seen, numpersons):
        frontierPersonIndices = frontier.to_lists()[0]
        hasCreatorTransposed = self.hasCreator.transpose()
        replyOfTransposed = self.replyOf.transpose()

        # # bad and ugly
        # sel = Matrix.from_lists(frontierPersonIndices, frontierPersonIndices, [1]*len(frontierPersonIndices), numpersons, numpersons)
        # if num_of_interactions >= 0:
        #     FreqComm1 = sel.mxm(knows).mxm(hasCreatorTransposed).mxm(replyOf          ).mxm(hasCreator, mask=knows).select(lib.GxB_GT_THUNK, num_of_interactions)
        #     FreqComm2 = sel.mxm(knows).mxm(hasCreatorTransposed).mxm(replyOfTransposed).mxm(hasCreator, mask=knows).select(lib.GxB_GT_THUNK, num_of_interactions)
        #     FreqComm = FreqComm1*FreqComm2
        #     FreqComm = FreqComm.transpose()
        #     next = FreqComm.reduce_vector().pattern()
        # else:
        #     next = frontier.vxm(knows)

        # good
        sel = Matrix.from_lists(frontierPersonIndices, frontierPersonIndices, [1] * len(frontierPersonIndices),
                                numpersons, numpersons)
        if self.num_of_interactions >= 0:
            FreqComm1 = sel.mxm(hasCreatorTransposed).mxm(self.replyOf).mxm(self.hasCreator, mask=self.knows).select(lib.GxB_GT_THUNK,
                                                                                                      self.num_of_interactions)
            FreqComm2 = sel.mxm(hasCreatorTransposed).mxm(replyOfTransposed).mxm(self.hasCreator, mask=self.knows).select(
                lib.GxB_GT_THUNK, self.num_of_interactions)
            FreqComm = FreqComm1 * FreqComm2
            FreqComm = FreqComm.transpose()
            next = FreqComm.reduce_vector().pattern()
        else:
            next = frontier.vxm(self.knows)

        # print(next, next.type)
        # print(next.to_string())

        return next

    def shortest_distance_over_frequent_communication_paths_opt(self, params):

        self.person1_id = params[0]
        self.person2_id = params[1]
        self.num_of_interactions = params[2]

        if self.person is None:
            # Load vertices and edges
            load_start = timer()
            self.person = self.loader.load_vertex('person')
            self.comment = self.loader.load_vertex('comment')
            self.replyOf = self.loader.load_edge('replyOf', self.comment, self.comment)
            self.knows = self.loader.load_edge('knows', self.person, self.person)
            self.hasCreator = self.loader.load_edge('hasCreator', self.comment, self.person)
            load_end = timer()
            self.load_time = load_end - load_start
            log.info(f'Loading took: {self.load_time} seconds')

        # Run query
        query_start = timer()
        person1_id_remapped = self.person.id2index[self.person1_id]
        person2_id_remapped = self.person.id2index[self.person2_id]

        numpersons = len(self.person.id2index)
        frontier1 = Vector.from_lists([person1_id_remapped], [True], numpersons)
        frontier2 = Vector.from_lists([person2_id_remapped], [True], numpersons)
        seen1 = frontier1
        seen2 = frontier2

        for level in range(1, numpersons // 2):
            # print("===== " + str(level) + " =====")
            # print("frontier persons: " + str(frontierPersonIndices))

            # frontier 1
            next1 = self.step_frontier(frontier1, seen1, numpersons)

            # emptied the component of person1
            if next1.nvals == 0:
                query_end = timer()
                self.test_execution_times.append(query_end - query_start)
                log.info(f'Query took: {query_end - query_start} second')
                return -1
            # has frontier1 intersected frontier2's previous state?
            #intersection1 = next1 * seen2
            intersection1 = next1 * frontier2
            if intersection1.nvals > 0:
                query_end = timer()
                self.test_execution_times.append(query_end - query_start)
                log.info(f'Query took: {query_end - query_start} second')
                return level * 2 - 1

            # frontier 2
            next2 = self.step_frontier(frontier2, seen2, numpersons)

            # emptied the component of person2
            if next2.nvals == 0:
                query_end = timer()
                self.test_execution_times.append(query_end - query_start)
                log.info(f'Query took: {query_end - query_start} second')
                return -1
            # do frontier1 and frontier2's current states intersect?
            intersection2 = next1 * next2
            if intersection2.nvals > 0:
                query_end = timer()
                self.test_execution_times.append(query_end - query_start)
                log.info(f'Query took: {query_end - query_start} second')
                return level * 2

            # step the frontiers
            seen1 = seen1 + next1
            frontier1 = next1

            seen2 = seen2 + next2
            frontier2 = next2

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
