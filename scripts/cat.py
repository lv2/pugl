#!/usr/bin/env python

import sys

for filename in sys.argv[1:]:
    with open(filename, 'r') as f:
        sys.stdout.write(f.read())
