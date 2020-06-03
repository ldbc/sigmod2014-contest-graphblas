from abc import ABC, abstractmethod
from collections import namedtuple

from loader.data_loader import DataLoader

Test = namedtuple('Test', ['inputs', 'expected_result'])


class QueryBase(ABC):
    def __init__(self, data_dir, data_format, tests):
        self.loader = DataLoader(data_dir, data_format)
        self.tests = tests

    @abstractmethod
    def execute_query(self):
        pass

    @abstractmethod
    def format_result_string(self):
        pass

    def run_tests(self):
        for test in self.tests:
            result = self.execute_query(test.inputs)
            result_string = self.format_result_string(result)
            result_correct = result_string == test.expected_result

            assert result_correct
