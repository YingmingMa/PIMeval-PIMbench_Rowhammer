#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
import sys
import pandas as pd
from collections import defaultdict
from pathlib import Path

if len(sys.argv) != 2:
    print("Usage:")
    print("./bitSerial.out > result.txt")
    print("./parseRowHammer.py result.txt > rowhammer.txt")
    print("./rowAccessperSecond.py rowhammer.txt > mostAccessperRI.txt")
    exit()

# Load file content
file_path = sys.argv[1]
with open(file_path, "r") as f:
    lines = f.readlines()

# Regular expressions for parsing
command_pattern = re.compile(r"PimCmdEnum::(\w+)")
time_pattern = re.compile(r"Estimated Time, ([\d.]+) ns")
core_row_pattern = re.compile(r"{ Core (\d+) , { Row: \[(.*?)\]")
row_access_pattern = re.compile(r"(\d+):(\d+)")

# Parse the file
command_data = {}
current_command = None

for line in lines:
    cmd_match = command_pattern.search(line)
    if cmd_match:
        current_command = cmd_match.group(1)
        command_data[current_command] = {"time_ns": 0.0, "row_accesses": defaultdict(list)}

    if current_command:
        time_match = time_pattern.search(line)
        if time_match:
            command_data[current_command]["time_ns"] = float(time_match.group(1))

        core_match = core_row_pattern.search(line)
        if core_match:
            core_id = int(core_match.group(1))
            row_data = core_match.group(2)
            row_access_counts = defaultdict(int)
            for row, count in row_access_pattern.findall(row_data):
                row_access_counts[int(row)] += int(count)
            command_data[current_command]["row_accesses"][core_id].append(row_access_counts)

# Aggregate results
results = []
for command, data in command_data.items():
    max_access = 0
    for core_data in data["row_accesses"].values():
        for row_dict in core_data:
            if row_dict:
                max_access = max(max_access, max(row_dict.values()))
    time_ns = data["time_ns"]
    time_ms = time_ns / 1e6 if time_ns else 0
    rate = max_access / time_ms if time_ms else 0
    rate64 = rate * 64
    results.append((command, max_access, time_ms, rate, rate64))

# Create DataFrame
df = pd.DataFrame(results, columns=["Command", "Max_Row_Access", "Runtime_ms", "Accesses_per_ms", "Accesses_per_RI"])
print(df.to_string(index=False))

