from queries.Query1 import Query1
from queries.Query2 import Query2
from queries.Query3 import Query3
from queries.Query4 import Query4

data_dir = 'csvs/o1k/'
data_format = 'csv'
q1 = Query1(data_dir, data_format)
q2 = Query2(data_dir, data_format)
q3 = Query3(data_dir, data_format)
q4 = Query4(data_dir, data_format)

tests_passed = True

tests_passed = q1.run_tests(q1.execute_query) and tests_passed
tests_passed = q2.run_tests(q2.execute_query) and tests_passed
tests_passed = q3.run_tests(q3.execute_query) and tests_passed
tests_passed = q4.run_tests(q4.execute_query) and tests_passed

if tests_passed:
    print('\nALL TESTS PASSED')
else:
    print('\nTESTS FAILED')
