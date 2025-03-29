#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import csv 
import re 
import sys

if len(sys.argv) != 2:
    print("Usage:")
    print("./bitSerial.out > result.txt")
    print("./parseRowHammer.py result.txt")
    exit()

filename = sys.argv[1]
# print("INFO: Bit-serial Micro-program Performance Results")

device = ""
data_type = ""
op = ""
prev_device = ""
prev_data_type = ""
core_data = {}  # will map core number (string) to {"row": list of (index, count), "col": list of (index, count)}
current_core = None
mode = None  # "row" or "col"

with open(filename, 'r') as f:
    for line in f:
        line = line.strip()
        
        # Look for test-case start header, e.g. "[bitsimd_v:int32:abs:0] Start"
        match_start = re.match(r'\[(.*):(.*):(.*):(.*)\]\s+Start', line)
        if match_start:
            prev_device = device
            device = match_start.group(1)
            # If device has changed, close previous device block.
            if device != prev_device:
                if prev_device:
                    print("  }},")
                print("  { PIM_DEVICE_%s, {" % device.upper())
            prev_data_type = data_type
            data_type = match_start.group(2)
            # If data type has changed, close previous data type block.
            if data_type != prev_data_type:
                if prev_data_type:
                    print("    }},")
                print("    { PIM_%s, {" % data_type.upper())
            op = match_start.group(3)
            # Reset core data for this operation.
            core_data = {}
            current_core = None
            mode = None
            continue

        # Look for core header e.g. "Memory Access Log for Core 0:"
        match_core = re.match(r'Memory Access Log for Core (\d+):', line)
        if match_core:
            current_core = match_core.group(1)
            core_data[current_core] = {"row": [], "col": []}
            mode = None
            continue

        # Detect the beginning of the row access block
        if line.startswith("Recorded Row Memory Accesses (CSV):"):
            mode = "row"
            continue

        # Detect the beginning of the column access block
        if line.startswith("Recorded Column Memory Accesses (CSV):"):
            mode = "col"
            continue

        # Skip header lines inside CSV blocks
        if line.startswith("RowIndex,") or line.startswith("ColIndex,"):
            continue

        # Look for test-case end header, e.g. "[bitsimd_v:int32:abs:0] End"
        match_end = re.match(r'\[(.*):(.*):(.*):(.*)\]\s+End', line)
        if match_end:
            # Output the collected data for this operation.
            # Use a formatting similar to the sample snippet.
            print("      { PimCmdEnum::%-13s { " % (op.upper() + ','))
            # Loop over cores in numeric order.
            for core in sorted(core_data.keys(), key=lambda x: int(x)):
                # Create a string with all row entries (format: index:count)
                row_str = ", ".join(["%s:%s" % (idx, count) for idx, count in core_data[core]["row"]])
                # Create a string with all column entries.
                col_str = ", ".join(["%s:%s" % (idx, count) for idx, count in core_data[core]["col"]])
                print("           { Core %-2s, { Row: [%s], Column: [%s] } }," % (core, row_str, col_str))
            print("      } },")
            continue

        # If inside a CSV block (row or column), parse the data lines.
        if mode in ("row", "col") and current_core is not None:
            # Skip blank lines.
            if not line:
                continue
            parts = line.split(',')
            if len(parts) == 2:
                idx = parts[0].strip()
                count = parts[1].strip()
                core_data[current_core][mode].append((idx, count))
            continue

# After processing all lines, close the remaining blocks.
print("    }},")
print("  }")
