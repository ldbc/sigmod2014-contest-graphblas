from collections import namedtuple

from queries.QueryBase import QueryBase
from timeit import default_timer as timer
import logging
import heapq
from pygraphblas import *


Test = namedtuple('Test', ['inputs', 'expected_result'])

handler = logging.StreamHandler()
handler.setFormatter(logging.Formatter('%(asctime)s %(levelname)-5s %(message)s'))
log = logging.getLogger(__name__)
log.propagate = False
log.addHandler(handler)
log.setLevel(logging.INFO)


class Query3(QueryBase):

    def __init__(self, data_dir, data_format):
        self.tests = self.init_tests()
        super().__init__(data_dir, data_format, self.tests)
        self.k = None
        self.h = None
        self.p = None
        self.person = None
        self.place = None
        self.organisation = None
        self.tag = None
        self.placeNames = None
        self.isPartOf = None
        self.personIsLocatedIn = None
        self.organisationIsLocatedIn = None
        self.workAt = None
        self.studyAt = None
        self.knows = None
        self.hasInterest = None

    def load_data(self):
        load_start = timer()
        self.person, _ = self.loader.load_vertex('person')
        self.place, extra_cols = self.loader.load_vertex('place', column_names=['name'])
        self.placeNames = extra_cols[0]
        self.organisation, _ = self.loader.load_vertex('organisation')
        self.tag, _ = self.loader.load_vertex('tag')
        self.isPartOf = self.loader.load_edge('isPartOf', self.place, self.place)
        self.personIsLocatedIn = self.loader.load_edge('isLocatedIn', self.person, self.place)
        self.organisationIsLocatedIn = self.loader.load_edge('isLocatedIn', self.organisation, self.place)
        self.workAt = self.loader.load_edge('workAt', self.person, self.organisation)
        self.studyAt = self.loader.load_edge('studyAt', self.person, self.organisation)
        self.knows = self.loader.load_edge('knows', self.person, self.person)
        self.hasInterest = self.loader.load_edge('hasInterest', self.person, self.tag)

        load_end = timer()
        self.load_time = load_end - load_start
        log.info(f'Loading took {self.load_time} seconds')

    def execute_query(self, params):
        self.k = params[0]
        self.h = params[1]
        self.p = params[2]

        if self.person is None:
            self.load_data()

        # Run query
        query_start = timer()

        persons_diag_mx = self.RelevantPeopleInPlaceMatrix(self.p)

        next_mx = persons_diag_mx
        seen_mx = next_mx.dup()

        # MSBFS
        for level in range(self.h):
            next_mx = next_mx.mxm(self.knows, mask=seen_mx, desc=descriptor.ooco)
            # emptied the component
            if next_mx.nvals == 0:
                break

            seen_mx = seen_mx + next_mx

        # source persons were filtered at the beginning
        # drop friends in different place
        h_reachable_knows_tril = seen_mx.offdiag().tril() @ persons_diag_mx

        # pattern: a hacky way to cast to UINT64 because count is required instead of existence
        resultMatrix = self.hasInterest.pattern(UINT64).mxm(self.hasInterest.transpose(), mask=h_reachable_knows_tril)
        result = heapq.nsmallest(self.k, resultMatrix, key=self.sortTriples)
        result_string = ''
        for res in result:
            person1_id = self.person.index2id[res[0]]
            person2_id = self.person.index2id[res[1]]

            if person1_id > person2_id:
                person1_id, person2_id = person2_id, person1_id

            result_string += f'{person1_id}|{person2_id} '

        if len(result) < self.k:
            result_zeros = heapq.nsmallest(self.k, h_reachable_knows_tril, key=self.sortTriples)
            remaining = self.k - len(result)
            while remaining > 0 and len(result_zeros) != 0:
                res = result_zeros.pop(0)
                if resultMatrix.get(res[0], res[1]) is None:
                    person1_id = self.person.index2id[res[0]]
                    person2_id = self.person.index2id[res[1]]

                    if person1_id > person2_id:
                        person1_id, person2_id = person2_id, person1_id

                    result_string += f'{person1_id}|{person2_id} '

                    remaining -= 1

        query_end = timer()
        self.test_execution_times.append(query_end - query_start)
        #log.info(f'Query took: {query_end - query_start} second')
        print(f'q3,{int(self.load_time*10**6)},{int((query_end - query_start)*10**6)},{result_string}')
        #log.info(result_string)
        return result_string

    def format_result_string(self, result):
        return result.split('%')[0]

    def sortTriples(self, triple):
        person1_id = self.person.index2id[triple[0]]
        person2_id = self.person.index2id[triple[1]]

        if person1_id > person2_id:
            person1_id, person2_id = person2_id, person1_id

        return -triple[2], person1_id, person2_id

    def RelevantPeopleInPlaceMatrix(self, placeName):
        placeIndex = self.placeNames.index(placeName)
        # Relevant places
        isPartOfTransposed = self.isPartOf.transpose()
        placeVector = Vector.from_type(BOOL, isPartOfTransposed.nrows)
        placeVector[placeIndex] = True
        relevantPlacesVector = placeVector + placeVector.vxm(isPartOfTransposed) + placeVector.vxm(
            isPartOfTransposed).vxm(isPartOfTransposed)
        # People located in the given place
        peopleInThePlaceVector = self.personIsLocatedIn.mxv(relevantPlacesVector)
        # People working at a Company or studying at a University located in the given place
        organisationsVector = self.organisationIsLocatedIn.mxv(relevantPlacesVector)
        with semiring.LOR_LAND_BOOL:
            peopleWorkAtVector = self.workAt.mxv(organisationsVector)
            peopleStudyAtVector = self.studyAt.mxv(organisationsVector)
            # All the relevant people in the given place
        with binaryop.PLUS_BOOL:
            relevantPeopleVector = peopleWorkAtVector + peopleStudyAtVector + peopleInThePlaceVector

            # Creating a diagonal matrix from the people ids
        diagMtx = Matrix.from_lists(relevantPeopleVector.to_lists()[0], relevantPeopleVector.to_lists()[0],
                                    relevantPeopleVector.to_lists()[1], self.knows.nrows, self.knows.ncols)
        return diagMtx

    def init_tests(self):
        tests = [
            Test([3, 2, 'Asia'], '361|812 174|280 280|812 % common interest counts 4 3 3'),
            Test([4, 3, 'Indonesia'], '396|398 363|367 363|368 363|372 % common interest counts 2 1 1 1'),
            Test([3, 2, 'Egypt'], '110|116 106|110 106|112 % common interest counts 1 0 0'),
            Test([3, 2, 'Italy'], '420|825 421|424 10|414 % common interest counts 1 1 0'),
            Test([5, 4, 'Chengdu'], '590|650 590|658 590|614 590|629 590|638 % common interest counts 1 1 0 0 0'),
            Test([3, 2, 'Peru'], '65|766 65|767 65|863 % common interest counts 0 0 0'),
            Test([3, 2, 'Democratic_Republic_of_the_Congo'], '99|100 99|101 99|102 % common interest counts 0 0 0'),
            Test([7, 6, 'Ankara'],
                 '891|898 890|891 890|895 890|898 890|902 891|895 891|902 % common interest counts 1 0 0 0 0 0 0'),
            Test([3, 2, 'Luoyang'], '565|625 653|726 565|653 % common interest counts 2 1 0'),
            Test([4, 3, 'Taiwan'], '795|798 797|798 567|795 567|796 % common interest counts 1 1 0 0')
        ]
        return tests
