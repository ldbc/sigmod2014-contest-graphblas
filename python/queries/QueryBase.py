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
        self.load_time = None
        self.test_execution_times = []

    @abstractmethod
    def execute_query(self, params):
        pass

    @abstractmethod
    def format_result_string(self, result):
        pass

    def run_tests(self):
        for test in self.tests:
            result = self.execute_query(test.inputs)
            test = test._replace(expected_result=self.format_result_string(test.expected_result))
            result_correct = result == test.expected_result

            assert result_correct
            log.info(f'Result: {result}')
            log.info(f'Result correct: {result_correct}')

    @abstractmethod
    def init_tests(self):
        pass
