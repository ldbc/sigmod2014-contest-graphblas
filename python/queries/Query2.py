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

        if self.person is None:
            # Load vertices and edges
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
                           for tagCol in range(len(self.tag.index2id))}

        result = heapq.nsmallest(self.top_k, tags_with_score, key=lambda kv: (-kv[1], kv[0]))

        query_end = timer()
        self.test_execution_times.append(query_end - query_start)
        log.info(f'Loading took: {self.load_time} seconds, Query took: {query_end - query_start} second')
        res_string = ''
        for res in result:
            res_string += res[0] + ' '
        res_string = res_string.replace('\\', '')
        return res_string

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
        return result.split('%')[0]

    def init_tests(self):
        tests = [
            Test([3, '1980-02-01'], 'Chiang_Kai-shek Augustine_of_Hippo Napoleon % component sizes 22 16 16'),
            Test([4, '1981-03-10'],
                 'Chiang_Kai-shek Napoleon Mohandas_Karamchand_Gandhi Sukarno % component sizes 17 13 11 11'),
            Test([3, '1982-03-29'],
                 'Chiang_Kai-shek Mohandas_Karamchand_Gandhi Napoleon % component sizes 13 11 10'),
            Test([3, '1983-05-09'],
                 'Chiang_Kai-shek Mohandas_Karamchand_Gandhi Augustine_of_Hippo % component sizes 12 10 8'),
            Test([5, '1984-07-02'],
                 'Chiang_Kai-shek Aristotle Mohandas_Karamchand_Gandhi Augustine_of_Hippo Fidel_Castro % component sizes 10 7 6 5 5'),
            Test([3, '1985-05-31'],
                 'Chiang_Kai-shek Mohandas_Karamchand_Gandhi Joseph_Stalin % component sizes 6 6 5'),
            Test([3, '1986-06-14'],
                 'Chiang_Kai-shek Mohandas_Karamchand_Gandhi Joseph_Stalin % component sizes 6 6 5'),
            Test([7, '1987-06-24'],
                 'Chiang_Kai-shek Augustine_of_Hippo Genghis_Khan Haile_Selassie_I Karl_Marx Lyndon_B._Johnson Robert_John_\"Mutt\"_Lange % component sizes 4 3 3 3 3 3 3'),
            Test([3, '1988-11-10'], 'Aristotle Ho_Chi_Minh Karl_Marx % component sizes 2 2 2'),
            Test([4, '1990-01-25'],
                 'Arthur_Conan_Doyle Ashoka Barack_Obama Benito_Mussolini % component sizes 1 1 1 1')
        ]
        return tests
