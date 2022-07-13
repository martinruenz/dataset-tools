from rna_xml import rna2xml
import os

def print_nodetree_xml(node_tree):
    rna2xml(root_rna=node_tree)

def save_nodetree_xml(node_tree, path):
    if os.path.exists(path):
        raise RuntimeError('file already exists')
    with open(path, 'w') as f:
        rna2xml(fw=file.write, root_rna=node_tree)
