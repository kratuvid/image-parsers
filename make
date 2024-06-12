#!/usr/bin/env python3

import time
import subprocess
import os
import sys

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

release_flags = ['-O3', '-DNDEBUG']
debug_flags = ['-g', '-DDEBUG']

dirs = {
    'sources': 'src',
    'build': 'build',
    'objects': 'obj',
    'sys-bmi': 'sys-bmi'
}

cxx = ['clang++']
cxx_flags = ['-fdiagnostics-color=always', '-std=c++23', '-Wno-experimental-header-units']
ld_flags = []

sys_modules = \
    ['print', 'string_view', 'format', 'exception', 'string', 'vector', 'variant', 'cstdint', 'memory',
     'fstream', 'cstring', 'cstddef', 'cstdlib', 'array', 'sstream', 'algorithm', 'limits', 'utility'] + \
    []

# Properties: is module, primary dependencies
primaries = {
    'logger': [[True, []], ['logger.cppm']],
    'image': [[True, ['logger']], ['image.cppm']],
    'main': [[False, ['image', 'logger']], ['main.cpp']]
}

targets = {
    'main': ['main', 'image', 'logger']
}

class Builder:
    args_nature = {
        'help': False,
        'release': False,
        'run': True,
        'show': False,
        'rebuild': False
    }

    def __init__(self):
        self.handle_arguments()

        self.make_directories()
        self.make_sys_modules()
        self.make_targets()
        self.run_targets()

    def run_targets(self):
        if 'run' in self.args:
            target = self.args['run']
            if target not in targets:
                raise Exception(f'Invalid target {target}')
            target_path = Builder.join_paths([ self.dirs['build'], target ])
            eprint('> Run:', target)
            self.run([target_path])

    def make_targets(self):
        self.primaries_checked = set()
        self.primaries_updated = set()

        for target in targets:
            target_path = Builder.join_paths([ self.dirs['build'], target ])

            for primary in targets[target]:
                self.secondaries_checked = set()
                self.secondaries_updated = set()
                self.module_flags = []

                self.make_primary(primary, self.force)

            if not Builder.is_exists(target_path) or len(self.primaries_updated) != 0 or self.force:
                eprint('> Link:', target, end=' ')
                begin = time.time()
                self.link(target, [])
                end = time.time()
                eprint(f'- {end-begin:.3e}s')

    def make_primary(self, primary, force=False):
        if primary in self.primaries_checked:
            return
        self.primaries_checked.add(primary)

        primary_sources = primaries[primary][1]
        is_module = primaries[primary][0][0]
        deps = primaries[primary][0][1]

        prev_pu = len(self.primaries_updated)
        for dep in deps:
            self.make_primary(dep, force)
        now_pu = len(self.primaries_updated)

        force = True if (now_pu > prev_pu or force) else False

        if is_module:
            # MIU is module interface unit
            primary_miu = primary_sources[0]
            module_flags = []

            prev_su = len(self.secondaries_updated)
            for unit_index in range(1, len(primary_sources)):
                source = primary_sources[unit_index]
                if source.endswith('.cppm'):
                    basename = source.removesuffix('.cppm')
                    bmi_name = primary + ':' + basename
                    bmi_path = Builder.join_paths([ self.dirs['objects'], primary, basename + '.pcm' ])
                    module_flags += ['-fmodule-file=' + bmi_name + '=' + bmi_path]
                self.make_secondary((primary, source), self.module_flags, force)
            now_su = len(self.secondaries_updated)

            force_internal = True if (now_su > prev_su or force) else False
            self.make_secondary((primary, primary_miu), self.module_flags + module_flags, force_internal)

            bmi_path = Builder.join_paths([ self.dirs['objects'], primary, primary + '.pcm' ])
            module_flags += ['-fmodule-file=' + primary + '=' + bmi_path]
            self.module_flags += module_flags

        else:
            for unit in primary_sources:
                self.make_secondary((primary, unit), self.module_flags, force)

        if len(self.secondaries_updated) != 0:
            self.primaries_updated.add(primary)

    def make_secondary(self, secondary, extra_flags, force=False):
        if secondary in self.secondaries_checked:
            return
        self.secondaries_checked.add(secondary)

        primary = secondary[0]
        unit = secondary[1]

        basename = Builder.removesuffixes(unit, ['.cpp', '.cppm'])
        path = Builder.join_paths([ dirs['sources'], primary, unit ])
        object_path = Builder.join_paths([ self.dirs['objects'], primary, basename + '.o' ])
        bmi_path = Builder.join_paths([ self.dirs['objects'], primary, basename + '.pcm' ])
        is_bmi = unit.endswith('.cppm')

        if (not Builder.is_exists(object_path) or Builder.is_later(path, object_path)) or (is_bmi and (not Builder.is_exists(bmi_path) or Builder.is_later(path, bmi_path))) or force:
            eprint('> Compile:', '/'.join([primary, unit]), end=' ')
            begin = time.time()
            if is_bmi:
                self.precompile(path, bmi_path, extra_flags)
            self.compile(path, object_path, extra_flags)
            end = time.time()
            eprint(f'- {end-begin:.3e}s')
            self.secondaries_updated.add(secondary)

    def make_sys_modules(self):
        for module in sys_modules:
            target = Builder.join_paths([ self.dirs['sys-bmi'], module + '.pcm' ])
            if module.endswith('.h'):
                target = Builder.join_paths([ self.dirs['sys-bmi'], module.replace('/', '-').removesuffix('.h') + '.pcm' ])

            if not Builder.is_exists(target):
                eprint('> Precompile system BMI:', module, end=' ')
                begin = time.time()
                self.run(cxx + cxx_flags + ['-Wno-pragma-system-header-outside-header', '--precompile', '-xc++-system-header', module, '-o', target])
                end = time.time()
                eprint(f'- {end-begin:.3e}s')

    def make_directories(self):
        os.makedirs(self.dirs['build'], exist_ok=True)
        os.makedirs(self.dirs['objects'], exist_ok=True)
        for primary in primaries:
            os.makedirs(Builder.join_paths([ self.dirs['objects'], primary ]), exist_ok=True)
        os.makedirs(self.dirs['sys-bmi'], exist_ok=True)

    def precompile(self, source, target, extra_flags):
        self.run(cxx + self.type_flags + self.base_flags + cxx_flags + extra_flags + ['--precompile', source, '-o', target])

    def compile(self, source, target, extra_flags):
        self.run(cxx + self.type_flags + self.base_flags + cxx_flags + extra_flags + ['-c', source, '-o', target])

    def link(self, target, extra_flags):
        target_path = Builder.join_paths([ self.dirs['build'], target ])

        objects = []
        for primary in targets[target]:
            for file in primaries[primary][1]:
                objects += [Builder.join_paths([ self.dirs['objects'], primary, Builder.replacesuffixes(file, ['.cpp', '.cppm'], '.o') ])]

        self.run(cxx + self.type_flags + self.base_flags + ld_flags + extra_flags + objects + ['-o', target_path])

    def removesuffixes(path, formers):
        for former in formers:
            path = path.removesuffix(former)
        return path

    def replacesuffixes(path, formers, later):
        return Builder.removesuffixes(path, formers) + later

    def join_paths(args):
        return '/'.join(args)

    def is_later(path, path2):
        return os.path.getmtime(path) > os.path.getmtime(path2)

    def is_exists(path):
        return os.path.exists(path)

    def run(self, args):
        if self.show:
            eprint(' '.join(args))
        status = subprocess.run(args)
        if status.returncode != 0:
            raise Exception(f'Last command failed with code {status.returncode}')

    def handle_arguments(self):
        i = 1
        self.args = {}
        while i < len(sys.argv):
            this = sys.argv[i]
            if this in self.args_nature:
                if self.args_nature[this] is True:
                    if i == len(sys.argv)-1:
                        raise Exception(f'Argument \'{this}\' needs a parameter')
                    self.args[this] = sys.argv[i+1]
                    i += 1
                else:
                    self.args[this] = True
            i += 1

        if 'help' in self.args:
            eprint(self.args_nature)
            exit(1)

        self.show = 'show' in self.args
        self.force = 'rebuild' in self.args

        self.type = 'release' if 'release' in self.args else 'debug'
        self.type_flags = release_flags if 'release' in self.args else debug_flags

        self.dirs = {'build': Builder.join_paths([ dirs['build'], self.type ])}
        self.dirs['objects'] = Builder.join_paths([ self.dirs['build'], dirs['objects'] ])
        self.dirs['sys-bmi'] = Builder.join_paths([ dirs['build'], dirs['sys-bmi'] ])

        self.base_flags = []
        for module in sys_modules:
            target = Builder.join_paths([ self.dirs['sys-bmi'], module + '.pcm' ])
            if module.endswith('.h'):
                target = Builder.join_paths([ self.dirs['sys-bmi'], module.replace('/', '-').removesuffix('.h') + '.pcm' ])
            self.base_flags += ['-fmodule-file=' + target]


if __name__ == '__main__':
    try:
        instance = Builder()
    except Exception as e:
        eprint('Exception caught:', e)
        exit(1)
