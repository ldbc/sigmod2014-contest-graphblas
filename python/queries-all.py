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

q1.run_tests()
q2.run_tests()
q3.run_tests()
q4.run_tests()
