#!/usr/bin/env python3

# Helper script to generate compile_commands.json from running Make
# If you don't want a compile_commands.json for your editor, running make is ok

from libscanbuild.compilation import Compilation, CompilationDatabase
from libscanbuild.arguments import create_intercept_parser
import itertools
from typing import *
import os
import sys

import subprocess
import argparse

def libscanbuild_capture(args: argparse.Namespace) -> Tuple[int, Iterable[Compilation]]:
    """
    Implementation of compilation database generation.
    :param args:    the parsed and validated command line arguments
    :return:        the exit status of build process.
    """
    from libscanbuild.intercept import setup_environment, run_build, exec_trace_files, parse_exec_trace, \
        compilations
    from libear import temporary_directory

    with temporary_directory(prefix='intercept-') as tmp_dir:
        # run the build command
        environment = setup_environment(args, tmp_dir)
        if os.environ.get('PROS_TOOLCHAIN'):
            environment['PATH'] = os.path.join(os.environ.get('PROS_TOOLCHAIN'), 'bin') + os.pathsep + \
                                    environment['PATH']

        if sys.platform == 'darwin':
            environment['PATH'] = os.path.dirname(os.path.abspath(sys.executable)) + os.pathsep + \
                                    environment['PATH']

        exit_code = run_build(args.build, env=environment)
        # read the intercepted exec calls
        calls = (parse_exec_trace(file) for file in exec_trace_files(tmp_dir))
        current = compilations(calls, args.cc, args.cxx)

        return exit_code, iter(set(current))

# call make.exe if on Windows
if os.name == 'nt' and os.environ.get('PROS_TOOLCHAIN'):
    make_cmd = os.path.join(os.environ.get('PROS_TOOLCHAIN'), 'bin', 'make.exe')
else:
    make_cmd = 'make'
args = create_intercept_parser().parse_args(
    ['--override-compiler', '--use-cc', 'arm-none-eabi-gcc', '--use-c++', 'arm-none-eabi-g++', make_cmd,
        *sys.argv[1:],
        'CC=intercept-cc', 'CXX=intercept-c++'])
exit_code, entries = libscanbuild_capture(args)

any_entries, entries = itertools.tee(entries, 2)
if not any(any_entries):
    sys.exit(exit_code)

print('Capturing metadata...')
env = os.environ.copy()
# Add PROS toolchain to the beginning of PATH to ensure PROS binaries are preferred
if os.environ.get('PROS_TOOLCHAIN'):
    env['PATH'] = os.path.join(os.environ.get('PROS_TOOLCHAIN'), 'bin') + os.pathsep + env['PATH']
cc_sysroot = subprocess.run([make_cmd, 'cc-sysroot'], env=env, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
lines = str(cc_sysroot.stderr.decode()).splitlines() + str(cc_sysroot.stdout.decode()).splitlines()
lines = [l.strip() for l in lines]
cc_sysroot_includes = []
copy = False
for line in lines:
    if line == '#include <...> search starts here:':
        copy = True
        continue
    if line == 'End of search list.':
        copy = False
        continue
    if copy:
        cc_sysroot_includes.append(f'-isystem{line}')
cxx_sysroot = subprocess.run([make_cmd, 'cxx-sysroot'], env=env, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
lines = str(cxx_sysroot.stderr.decode()).splitlines() + str(cxx_sysroot.stdout.decode()).splitlines()
lines = [l.strip() for l in lines]
cxx_sysroot_includes = []
copy = False
for line in lines:
    if line == '#include <...> search starts here:':
        copy = True
        continue
    if line == 'End of search list.':
        copy = False
        continue
    if copy:
        cxx_sysroot_includes.append(f'-isystem{line}')
new_entries, entries = itertools.tee(entries, 2)
new_sources = set([e.source for e in entries])
cdb_file = 'compile_commands.json'
if isinstance(cdb_file, str) and os.path.isfile(cdb_file):
    old_entries = itertools.filterfalse(lambda entry: entry.source in new_sources,
                                        CompilationDatabase.load(cdb_file))
else:
    old_entries = []

extra_flags = ['-target', 'armv6m-none-none-eabi']

if sys.platform == 'win32':
    extra_flags.extend(["-fno-ms-extensions", "-fno-ms-compatibility", "-fno-delayed-template-parsing"])

def new_entry_map(entry):
    if entry.compiler == 'c':
        entry.flags = extra_flags + cc_sysroot_includes + entry.flags
    elif entry.compiler == 'c++':
        entry.flags = extra_flags + cxx_sysroot_includes + entry.flags
    return entry

new_entries = map(new_entry_map, new_entries)

def entry_map(entry: Compilation):
    json_entry = entry.as_db_entry()
    json_entry['arguments'][0] = 'clang' if entry.compiler == 'cc' else 'clang++'
    return json_entry

entries = itertools.chain(old_entries, new_entries)
json_entries = list(map(entry_map, entries))
if isinstance(cdb_file, str):
    cdb_file = open(cdb_file, 'w')
import json
json.dump(json_entries, cdb_file, sort_keys=True, indent=4)

sys.exit(exit_code)