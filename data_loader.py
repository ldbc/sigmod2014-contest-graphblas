import csv
import sys
import logging
from pygraphblas import *
import glob
import errno
import os


# Setup logger
handler = logging.StreamHandler()
handler.setFormatter(logging.Formatter('%(asctime)s %(levelname)-5s %(message)s'))
log = logging.getLogger(__name__)
log.addHandler(handler)
log.setLevel(logging.INFO)

Vertex = namedtuple('Vertex', ['name', 'id2index', 'index2id'])

class DataLoader:
    
    def __init__(self, data_dir, data_format):
        if os.path.isdir(data_dir):
            self.data_dir = data_dir
            self.data_format = '.' + data_format
        else:
            raise FileNotFoundError(errno.ENOENT, os.strerror(errno.ENOENT), data_dir)
        
    def load_vertex(self, vertex):
        filename = self.data_dir + vertex + self.data_format
        with open(filename, newline='') as csvfile:
            reader = csv.DictReader(csvfile, delimiter='|', quotechar='"')
            headers = reader.fieldnames
            log.info(f'Loading {filename} with headers: {headers}')
            node_key = headers[0]
            original_ids = [int(row[node_key]) for row in reader]
            id_mapping = {}
            for index in range(len(original_ids)):
                id_mapping[original_ids[index]] = index

        return Vertex(vertex, original_ids, id_mapping)

    
    def load_extra_columns(self, vertex, column_names):
        filename = self.data_dir + vertex + self.data_format
        with open(filename, newline='') as csv_file:
            reader = csv.DictReader(csv_file, delimiter='|', quotechar='"')

            full_column_names = []
            for columnName in column_names:
                # find full column name with type info after ':'
                full_column_name, = [fullName for fullName in reader.fieldnames
                                   if fullName.split(':')[0]==columnName]
                full_column_names.append(full_column_name)

            if len(full_column_names) == 1:
                full_column_name = full_column_names[0]
                result = [row[full_column_name] for row in reader]
            else:
                result = [[row[fullColumnName] for fullColumnName in full_column_names] for row in reader]

            return result
        
    def load_edge(self, edge_name, from_vertex, to_vertex, typ=INT64, drop_dangling_edges=False):
        start_mapping = from_vertex.index2id
        end_mapping = to_vertex.index2id
        
        filename = self.data_dir + from_vertex.name + '_' + edge_name + '_' + to_vertex.name + self.data_format
        with open(filename, newline='') as csvfile:
            reader = csv.DictReader(csvfile, delimiter='|', quotechar='"')
            row_ids = []
            col_ids = []
            values = []
            headers = reader.fieldnames
            log.info(f'Loading {filename} with headers: {headers}')
            start_key = headers[0]
            end_key = headers[1]
            for row in reader:
                start_id = int(row[start_key])
                end_id = int(row[end_key])
                if not drop_dangling_edges or (start_id in start_mapping and end_id in end_mapping):
                    row_ids.append(start_mapping[start_id])
                    col_ids.append(end_mapping[end_id])
                    values.append(1)
        
            edge_matrix = Matrix.from_lists(
            row_ids,
            col_ids,
            values,
            nrows=len(start_mapping), 
            ncols=len(end_mapping), 
            typ=typ)
            return edge_matrix
        
        
    def load_all_csvs(self):
        '''
        Loads all .csv files in the data directory defined in the DataLoader class
        
        Input files are expected in the following format:
            Nodes: nodeName.csv
            Edges: fromNodeName_edgeName_toNodeName.csv
        
        Returns:
            vertices: Dictionary containing the original Ids of vertices
            mappings: Dictionary containing the remapped Ids of vertices
            matrices: Dictionary containing the adjacency matrices with edge_names as keys
        
        '''
        files = glob.glob(f'{self.data_dir}*.csv')
        vertices = {}
        mappings = {}
        matrices = {}
        node_files = [file for file in files if '_' not in file]
        edge_files = [file for file in files if '_' in file]
        
        # Load nodes
        log.info('Loading nodes...')
        for node_file in node_files:
            name = node_file.replace(self.data_dir, '').replace('.csv', '')
            vertices[name], mappings[name] = self.load_vertex(node_file)
        # Load edges
        log.info('Loading edges...')
        for edge_file in edge_files:
            from_node, edge_name, to_node = edge_file.split('_')
            from_node = from_node.replace(self.data_dir, '')
            to_node = to_node.replace('.csv', '')    
            matrices[edge_name] = self.load_edge(edge_file, mappings[from_node], mappings[to_node])
        log.info('Finished loading edge data!')
        return vertices, mappings, matrices