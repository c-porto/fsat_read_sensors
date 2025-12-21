#!/usr/bin/env python3

import subprocess
import sys
from pathlib import Path

out = Path(sys.argv[1])

try:
    git_hash = subprocess.check_output(
        ['git', 'rev-parse', '--short', 'HEAD'],
        stderr=subprocess.DEVNULL
    ).decode().strip()
except Exception:
    git_hash = 'unknown'

project_version = sys.argv[2]

out.write_text(f'''\
#pragma once
#include <string_view>

inline constexpr const char* PROJECT_VERSION = "{project_version}+{git_hash}";
''')

