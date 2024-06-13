#!/usr/bin/env python3

import json
import os
import re
import sys

from node import Node
from utility import Type
from classify import classify

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)
    
class Baker:
    args_nature = {
        'help': 0,
        'release': 0,
        'run': 1,
        'show': 0,
        'rebuild': 0,
        'default_bakerfile': 0,
        'dump_bakerfile': 0
    }

    options_default = {
        'dirs': {
            'source': 'src',
            'build': 'build',
            'object': 'obj',
            'header_units': 'header_units',
        },
        'flags': {
            'debug': ['-g', '-DDEBUG'],
            'release': ['-O3', '-DNDEBUG'],
        },
        'options': {
        },
    }

    def __init__(self):
        self.load_bakerfile()
        self.handle_args()
        self.make_targets()

    def make_targets(self):
        targets = self.options['targets']
        if type(targets) != dict:
            raise ValueError(f'Targets must be a dict: {targets}')

        for target in targets:
            sources = targets[target]
            if type(sources) != list:
                raise ValueError(f'Source of each target must be a list (of strings): {targets}')

            self.gen_classes(sources)
            self.build_dependency_tree()

    def build_dependency_tree(self):
        pass

    def gen_classes(self, sources):
        os.chdir(self.options['dirs']['source'])
        
        self.classes = {
            Type.plain: set(),
            Type.module: {},
            Type.module_partition: {},
            Type.module_impl: {}
        }
        self.header_units = set()

        for index, source in enumerate(sources):
            if not (source.endswith('.cpp') or source.endswith('.cppm')):
                raise ValueError('Only .cpp and .cppm files are permitted as the values of targets')

            data = classify(source)

            self.header_units.update(data['header_units'])
            children = data['post']
            module = data['module']
            if True:
                del data['header_units']
                del data['post']

            node = Node(None, children, **data)
            print(node)

            if data['type'] == Type.plain:
                self.classes[Type.plain].add(node)
            elif data['type'] in [Type.module, Type.module_partition, Type.module_impl]:
                self.classes[data['type']][module] = node
            else:
                raise RuntimeError(f'Unknown utility.Type. This shouldn\'t have happened: {data["type"]}')

            if index == 0:
                if data['type'] != Type.plain or not source.endswith('.cpp'):
                    raise ValueError('Source represeting the target (i.e. the very first item in the list) must be plain')
                self.root_node = node

        os.chdir('..')

    def load_bakerfile(self):
        if not os.path.exists('bakerfile'):
            raise RuntimeError('Missing bakerfile')

        with open('bakerfile') as bakerfile:
            self.options = {}
            options = json.load(bakerfile)

            if 'targets' not in options:
                raise ValueError('Must specify targets')
            
            for key in self.options_default:
                if key in options:
                    self.options[key] = {}
                    for subkey in self.options_default[key]:
                        if subkey in options[key]:
                            value = options[key][subkey]
                            value_def = self.options_default[key][subkey]
                            if type(value) != type(value_def):
                                raise TypeError(f'{key} > {subkey} must be of type {type(value_def)}')
                            self.options[key][subkey] = value
                        else:
                            self.options[key][subkey] = self.options_default[key][subkey]
                else:
                    self.options[key] = self.options_default[key]

            for key in options:
                if key not in self.options:
                    self.options[key] = options[key]

    def handle_args(self):
        self.parse_args()
        self.process_args()

    def process_args(self):
        if 'help' in self.args:
            eprint(self.args_nature)
            exit(0)

        if 'default_bakerfile' in self.args:
            print(json.dumps(self.options_default, indent=4))
            exit(0)

        if 'dump_bakerfile' in self.args:
            print(json.dumps(self.options, indent=4))
            exit(0)

        self.show = 'show' in self.args
        self.rebuild = 'rebuild' in self.args

        self.type = 'release' if 'release' in self.args else 'debug'
        self.type_flags = self.options['flags']['release'] if self.type == 'release' else self.options['flags']['debug']

    def parse_args(self):
        i = 1
        self.args = {}

        while i < len(sys.argv):
            current = sys.argv[1]
            if current in self.args_nature:
                count = self.args_nature[current]
                if i + count >= len(sys.argv):
                    raise ValueError(f'{current} requires {count} arguments')
                self.args[current] = sys.argv[i + 1 : i + count + 1]
                i += count
            else:
                raise ValueError(f'Unknown argument: {current}')
            i += 1

if __name__ == '__main__':
    ins = Baker()
