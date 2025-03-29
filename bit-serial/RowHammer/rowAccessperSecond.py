#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
import sys
import pandas as pd
from collections import defaultdict

# File paths from command-line arguments
execution_stats_path = sys.argv[1]
row_access_path = sys.argv[2]

# Patterns
command_pattern = re.compile(r"PimCmdEnum::(\w+)")
core_row_pattern = re.compile(r"{ Core (\d+) , { Row: \[(.*?)\]")
row_access_pattern = re.compile(r"(\d+):(\d+)")

# Step 1: Parse row_access_path to get the most accessed row count per command
command_max_access = {}

with open(row_access_path, "r") as f:
    lines = f.readlines()

    current_command = None
    for line in lines:
        cmd_match = command_pattern.search(line)
        if cmd_match:
            current_command = cmd_match.group(1)
            command_max_access[current_command] = []

        if current_command:
            core_match = core_row_pattern.search(line)
            if core_match:
                row_data = core_match.group(2)
                row_access_counts = defaultdict(int)
                for row, count in row_access_pattern.findall(row_data):
                    row_access_counts[int(row)] += int(count)
                if row_access_counts:
                    max_access = max(row_access_counts.values())
                    command_max_access[current_command].append(max_access)

# Calculate final access count per command (the highest access count among all cores)
access_counts = {
    cmd: max(accesses) if accesses else 0
    for cmd, accesses in command_max_access.items()
}

# Step 2: Parse execution stats to get runtime per command
execution_times = {}
with open(execution_stats_path, "r") as f:
    for line in f:
        if ".int32.v" in line: #todo: can make more generic thus can parse other data
            parts = re.split(r'\s{2,}', line.strip())
            if len(parts) >= 4:
                name = parts[0].split(".")[0].upper()
                runtime_ms = float(parts[2])
                execution_times[name] = runtime_ms

# Step 3: Combine and calculate access rate
results = []
for command, access_count in access_counts.items():
    runtime = execution_times.get(command, None)
    if runtime is not None and runtime > 0:
        rate = access_count / runtime
        rate64 = rate*64 #todo: make the 64 modifieable
        results.append((command, access_count, runtime, rate, rate64))

# Step 4: Output
df = pd.DataFrame(results, columns=["Command", "Max_Row_Access", "Runtime_ms", "Accesses_per_ms", "Accesses_per_RI"])
print(df.to_string(index=False))
