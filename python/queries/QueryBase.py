from abc import ABC, abstractmethod


class QueryBase(ABC):
    def __init__(self):
        pass

    @abstractmethod
    def execute_query(self):
        pass

    @abstractmethod
    def run_tests(self):
        pass
