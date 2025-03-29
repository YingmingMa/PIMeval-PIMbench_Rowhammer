#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
import sys
from collections import defaultdict

# Reorganized and improved script as requested
filename = sys.argv[1]
print("INFO: Bit-serial Micro-program Performance Results\n")

with open(filename, 'r') as f:
    lines = f.readlines()

row_access_pattern = re.compile(r"(\d+):(\d+)")
command_pattern = re.compile(r"PimCmdEnum::(\w+)")
core_pattern = re.compile(r"{ Core (\d+) , { Row: \[(.*?)\],")

current_command = None
command_results = {}

for line in lines:
    # Detect command
    cmd_match = command_pattern.search(line)
    if cmd_match:
        current_command = cmd_match.group(1)
        command_results[current_command] = {}

    # Parse each core's row accesses
    if current_command:
        core_match = core_pattern.search(line)
        if core_match:
            core_id = int(core_match.group(1))
            row_data = core_match.group(2)

            row_access_counts = defaultdict(int)
            matches = row_access_pattern.findall(row_data)
            for row, count in matches:
                row_access_counts[int(row)] += int(count)

            # Identify the row with the most accesses (tie-breaking with highest row index)
            if row_access_counts:
                most_accessed_row = max(
                    row_access_counts.items(),
                    key=lambda x: (x[1], x[0])
                )
                command_results[current_command][core_id] = most_accessed_row

# Print results
for command, core_data in command_results.items():
    print(f"Command: {command}")
    for core_id, (row, count) in sorted(core_data.items()):
        print(f"  Core {core_id}: Row {row} with {count} accesses")
    print()
