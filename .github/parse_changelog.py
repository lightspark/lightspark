#!/usr/bin/env python3
#
# Parse the ChangeLog and output a release body with the current version's
# bullet points.
#
# Return 0 upon success, 1 upon an error, 2 upon incorrect usage.
#

import sys

if len(sys.argv) != 3:
    print("Usage: {} <version> <changelog>".format(sys.argv[0]),
          file=sys.stderr)
    sys.exit(2)

current_version = sys.argv[1]
current_items = []
version_found = False
with open(sys.argv[2]) as changelog:
    for line in changelog:
        if not version_found and line.startswith('Version ' + current_version):
            version_found = True
            sys.stdout.write("Changes:\n")
        elif version_found and 'Version' in line:
            break
        elif version_found:
            sys.stdout.write(line)
if version_found:
    print("Additional notes go here, if any.")
else:
    print("Error: version {} not found in changelog. Was it updated?".format(
        current_version), file=sys.stderr)
    sys.exit(1)
