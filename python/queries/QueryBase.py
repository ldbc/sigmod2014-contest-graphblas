from abc import ABC, abstractmethod
from collections import namedtuple
import logging
from loader.data_loader import DataLoader

Test = namedtuple('Test', ['inputs', 'expected_result'])

handler = logging.StreamHandler()
handler.setFormatter(logging.Formatter('%(asctime)s %(levelname)-5s %(message)s'))
log = logging.getLogger(__name__)
log.propagate = False
log.addHandler(handler)
log.setLevel(logging.INFO)


class QueryBase(ABC):
    def __init__(self, data_dir, data_format, tests):
        self.loader = DataLoader(data_dir, data_format)
        self.tests = tests
        self.benchmark_tests = None
        self.load_time = None
        self.test_execution_times = []

    @abstractmethod
    def execute_query(self, params):
        pass

    @abstractmethod
    def load_data(self):
        pass

    @abstractmethod
    def format_result_string(self, result):
        pass

    def run_tests(self, query_implementation, mode='testing'):
        # To run the tests of Q1 and Q4 with different search methods,
        # change the default parameter in their execute_query function
        all_tests = 0
        failed_tests = 0
        if mode is 'testing':
            tests = self.tests
        elif mode is 'benchmark':
            tests = self.benchmark_inputs
        else:
            raise Exception('Invalid mode for executing tests. Valid options are: [testing, benchmark]')
        log.info(f'Running tests in {mode} mode')
        for test in tests:
            result = query_implementation(test.inputs)
            test = test._replace(expected_result=self.format_result_string(test.expected_result))
            if result == test.expected_result:
                log.info(f'Result: {result}')
                log.info(f'TEST PASSED')
                log.info('----------------------')
            else:
                failed_tests += 1
                log.error(f'TEST FAILED: result: {result}  expected result: {test.expected_result}')
            all_tests += 1

        if failed_tests == 0:
            log.info(f'ALL TESTS PASSED: {all_tests}')
            return True
        else:
            log.error(f'TESTS FAILED: {failed_tests}/{all_tests}')
            return False

    @abstractmethod
    def init_tests(self):
        pass

    def init_benchmark_inputs(self, benchmark_tests):
        self.benchmark_inputs = benchmark_tests
