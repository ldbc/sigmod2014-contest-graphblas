from collections import namedtuple
from timeit import default_timer as timer
import logging
import heapq

from pygraphblas import *
from pygraphblas.base import lib
from pygraphblas.lagraph import LAGraph_cc_fastsv

from queries.QueryBase import QueryBase


Test = namedtuple('Test', ['inputs', 'expected_result'])

handler = logging.StreamHandler()
handler.setFormatter(logging.Formatter('%(asctime)s %(levelname)-5s %(message)s'))
log = logging.getLogger(__name__)
log.propagate = False
log.addHandler(handler)
log.setLevel(logging.INFO)


class Query2(QueryBase):

    def __init__(self, data_dir, data_format):
        self.tests = self.init_tests()
        super().__init__(data_dir, data_format, self.tests)
        self.top_k = None
        self.birthday_limit = None
        self.person = None
        self.tag = None
        self.hasInterest = None
        self.knows = None
        self.hasInterest_tran_mx = None
        self.personBirthdays = None
        self.tagNames = None

    def execute_query(self, params):
        self.top_k = params[0]
        self.birthday_limit = params[1]

        if self.person is not None:
            load_start = timer()
            self.person = self.loader.load_vertex('person')
            self.tag = self.loader.load_vertex('tag')
            self.hasInterest = self.loader.load_edge('hasInterest', self.person, self.tag)
            self.knows = self.loader.load_edge('knows', self.person, self.person)
            self.hasInterest_tran_mx = self.hasInterest.transpose()
            self.personBirthdays = self.loader.load_extra_columns('person', ['birthday'])
            self.tagNames = self.loader.load_extra_columns('tag', ['name'])
            load_end = timer()
            self.load_time = load_end - load_start

        # Run query
        query_start = timer()

        is_person_selected = [birthday >= self.birthday_limit for birthday in self.personBirthdays]
        birthday_person_mask = Vector.from_list(is_person_selected)
        birthday_person_mask.select(lib.GxB_NONZERO, out=birthday_person_mask)

        tags_with_score = {(self.tagNames[tagCol], self.get_score_for_tag(tagCol, birthday_person_mask))
                           for tagCol in range(len(self.tag.id2index))}

        result = heapq.nsmallest(self.top_k, tags_with_score, key=lambda kv: (-kv[1], kv[0]))

        query_end = timer()
        self.test_execution_times.append(query_end - query_start)
        log.info(f'Loading took: {self.load_time} seconds, Query took: {query_end - query_start} second')
        return result

    def get_score_for_tag(self, tag_index, birthday_person_mask):
        person_vec = self.hasInterest_tran_mx[tag_index]
        person_vec *= birthday_person_mask

        person_cols_in_subgraph, _ = person_vec.to_lists()
        person_count_in_subgraph = len(person_cols_in_subgraph)

        subgraph_mx = self.knows[person_cols_in_subgraph, person_cols_in_subgraph]

        _, component_ids = LAGraph_cc_fastsv(subgraph_mx, False).to_lists()

        component_sizes = [0] * person_count_in_subgraph
        for componentId in component_ids:
            component_sizes[componentId] += 1

        max_component_size = max(component_sizes) if component_sizes \
            else 0

        return max_component_size

    def format_result_string(self, result):
        pass

    def run_tests(self):
        pass

    def init_tests(self):
        return 0
