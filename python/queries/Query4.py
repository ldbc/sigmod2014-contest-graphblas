from collections import namedtuple

from queries.QueryBase import QueryBase
import logging
from pygraphblas import *
from timeit import default_timer as timer
from algorithms.search import naive_bfs_levels, push_pull_bfs_levels, msbfs_levels, push_pull_msbfs_levels
import operator

# Setup logger
handler = logging.StreamHandler()
handler.setFormatter(logging.Formatter('%(asctime)s %(levelname)-5s %(message)s'))
log = logging.getLogger(__name__)
log.propagate = False
log.addHandler(handler)
log.setLevel(logging.INFO)

Test = namedtuple('Test', ['inputs', 'expected_result'])


class Query4(QueryBase):

    def __init__(self, data_dir, data_format):
        self.tests = self.init_tests()
        super().__init__(data_dir, data_format, self.tests)
        self.k = None
        self.t = None
        self.person = None
        self.forum = None
        self.tag = None
        self.tagNames = None
        self.knows = None
        self.hasTag = None
        self.hasMember = None

    def load_data(self):
        load_start = timer()
        self.person, _ = self.loader.load_vertex('person')
        self.forum, _ = self.loader.load_vertex('forum')
        self.tag, extra_cols = self.loader.load_vertex('tag', column_names=['name'])
        self.tagNames = extra_cols[0]
        self.knows = self.loader.load_edge('knows', self.person, self.person)
        self.hasTag = self.loader.load_edge('hasTag', self.forum, self.tag)
        self.hasMember = self.loader.load_edge('hasMember', self.forum, self.person)

        load_end = timer()
        self.load_time = load_end - load_start
        log.info(f'Loading took {self.load_time} seconds')

    def execute_query(self, params, search_method=push_pull_msbfs_levels):
        self.k = params[0]
        self.t = params[1]
        if self.person is None:
            self.load_data()

        # Run query
        query_start = timer()
        if search_method is naive_bfs_levels or search_method is push_pull_bfs_levels:
            matrix, idList = self.MemberFriends(self.t)
            resultList = []
            for value in idList:
                bfsResultVector = search_method(matrix, value[0])
                n = len(idList)
                r = bfsResultVector.nvals
                s = 0
                for entry in bfsResultVector:
                    s += entry[1] - 1
                if s == 0 or n - 1 == 0:
                    score = 0
                else:
                    score = ((r - 1) * (r - 1)) / ((n - 1) * s)
                resultList += [[self.person.index2id[value[1]], score]]

            resultList.sort(key=operator.itemgetter(0))
            resultList.sort(key=operator.itemgetter(1), reverse=True)

            query_end = timer()
            self.test_execution_times.append(query_end - query_start)
            log.info(f'Query took: {query_end - query_start} second')

            result_string = ''
            for res in resultList[:self.k]:
                result_string += str(res[0]) + ' '
            #log.info(result_string)
            print(f'q4,{self.load_time*10**6},{(query_end - query_start)*10**6},{result_string}')
            return result_string

        else:  # search method is naive_msbfs or push_pull_msbfs
            matrix, idList = self.MemberFriends(self.t)
            i = matrix.nrows
            sources = Matrix.from_lists(range(i), range(i), [True] * i)
            resultMatrix = search_method(matrix, sources)
            resultList = []
            n = resultMatrix.nrows
            for value in idList:
                vec = resultMatrix.extract_row(value[0])
                r = vec.nvals
                s = vec.reduce_int()

                if s == 0 or n - 1 == 0:
                    score = 0
                else:
                    score = ((r - 1) * (r - 1)) / ((n - 1) * s)

                resultList += [[self.person.index2id[value[1]], score]]

            resultList.sort(key=operator.itemgetter(0))
            resultList.sort(key=operator.itemgetter(1), reverse=True)

            query_end = timer()
            self.test_execution_times.append(query_end - query_start)
            #log.info(f'Query took: {query_end - query_start} second')

            result_string = ''
            for res in resultList[:self.k]:
                result_string += str(res[0]) + ' '
            print(f'q4,{self.load_time*10**6},{(query_end - query_start)*10**6},{result_string}')
            return result_string

    def MemberFriends(self, t):
        tagIndex = self.tagNames.index(t)
        tagVector = Vector.from_type(BOOL, self.hasTag.ncols)
        tagVector[tagIndex] = True
        relevantForumsVector = tagVector.vxm(self.hasTag.transpose())
        relevantPeopleVector = relevantForumsVector.vxm(self.hasMember)
        resultMatrix = Matrix.from_type(BOOL, relevantPeopleVector.nvals, relevantPeopleVector.nvals)
        self.knows.extract_matrix(relevantPeopleVector.to_lists()[0],
                                  relevantPeopleVector.to_lists()[0],
                                  out=resultMatrix)
        n = 0
        idList = []
        for value in relevantPeopleVector:
            idList += [[n, value[0]]]
            n += 1

        return resultMatrix, idList

    def format_result_string(self, result):
        return result.split('%')[0]

    def init_tests(self):
        tests = [
            Test([3, 'Bill_Clinton'], '385 492 819 % centrality values 0.5290135396518375 0.5259615384615384 0.5249520153550864'),
            Test([4, 'Napoleon'], '722 530 366 316 % centrality values 0.5411255411255411 0.5405405405405406 0.5387931034482758 0.5382131324004306'),
            Test([3, 'Chiang_Kai-shek'], '592 565 625 % centrality values 0.5453460620525059 0.5421115065243179 0.5408284023668639'),
            Test([3, 'Charles_Darwin'], '458 305 913 % centrality values 0.5415676959619953 0.5371024734982333 0.5345838218053928'),
            Test([5, 'Ronald_Reagan'], '953 294 23 100 405 % centrality values 0.5446859903381642 0.5394736842105263 0.5388291517323776 0.5375446960667462 0.5343601895734598'),
            Test([3, 'Aristotle'], '426 819 429 % centrality values 0.5451219512195121 0.5424757281553397 0.5366146458583433'),
            Test([3, 'George_W._Bush'], '323 101 541 % centrality values 0.553596806524207 0.5433196380862576 0.5413098243818695'),
            Test([7, 'Tony_Blair'], '465 647 366 722 194 135 336 % centrality values 0.535629340423861 0.5317101013475888 0.5310624641961301 0.530416402804164 0.5297719114277313 0.5259376153257211 0.5253039555482203'),
            Test([3, 'William_Shakespeare'], '424 842 23 % centrality values 0.5316276893212858 0.5296265813313688 0.5256692220686188'),
            Test([4, 'Augustine_of_Hippo'], '385 562 659 323 % centrality values 0.5506329113924051 0.54375 0.54375 0.5291970802919708')
        ]
        return tests

