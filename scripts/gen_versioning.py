#!/usr/bin/env python3

import subprocess
import sys

project_version = sys.argv[1]

try:
    git_hash = subprocess.check_output(
        ['git', 'rev-parse', '--short', 'HEAD'],
        stderr=subprocess.DEVNULL
    ).decode().strip()

    dirty = subprocess.call(
        ['git', 'diff', '--quiet', 'HEAD', '--'],
        stderr=subprocess.DEVNULL
    ) != 0

    suffix = f'+{git_hash}{"-dirty" if dirty else ""}'
except Exception:
    suffix = ''

print(f'{project_version}{suffix}', end='')
