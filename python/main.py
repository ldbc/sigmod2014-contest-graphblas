import argparse


from queries.Query1 import Query1
from queries.Query2 import Query2
from queries.Query3 import Query3
from queries.Query4 import Query4


if __name__ == '__main__':

    argparse_formatter = lambda prog: argparse.RawDescriptionHelpFormatter(prog, max_help_position=60, width=250)

    parser = argparse.ArgumentParser(formatter_class=argparse_formatter, description='Run tool to benchmark queries')

    parser.add_argument('--data_path', default='../csvs/o1k/', help='[REQUIRED] Data to use (vertices and edges)')
    parser.add_argument('--query_args_path', default='../csvs/o1k/', help='[REQUIRED] Data to use (vertices and edges)')
    parser.add_argument('--queryies_to_run', default='all', choices=['1', '2', '3', '4', 'all'], help='[REQUIRED] Query to run')

    args = parser.parse_args()

    data_dir = args.data_path
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
