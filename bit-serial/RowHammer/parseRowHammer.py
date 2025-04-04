#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import csv 
import re 
import sys

if len(sys.argv) != 2:
    print("Usage:")
    print("./bitSerial.out > result.txt")
    print("./parseRowHammer.py result.txt > rowhammer.txt")
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

# Timing-related variables to compute execution time
timing_info = {
    "row_read_ns": 0.0,
    "row_write_ns": 0.0,
    "tccd_ns": 0.0,
    "num_read": 0,
    "num_write": 0,
    "num_logic": 0
}

with open(filename, 'r') as f:
    print("INFO: Bit-serial Micro-program Performance Results")
    for line in f:
        line = line.strip()

         # Parse timing values from header
        if "Row Read (ns)" in line:
            timing_info["row_read_ns"] = float(line.split(":")[1])
        elif "Row Write (ns)" in line:
            timing_info["row_write_ns"] = float(line.split(":")[1])
        elif "tCCD (ns)" in line:
            timing_info["tccd_ns"] = float(line.split(":")[1])

        # Parse operation counts from final summary line
        elif "Num Read, Write, Logic" in line:
            parts = re.findall(r'\d+', line)
            if len(parts) == 3:
                timing_info["num_read"] = int(parts[0])
                timing_info["num_write"] = int(parts[1])
                timing_info["num_logic"] = int(parts[2])
        
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
            # Calculate execution time in nanoseconds
            exec_time_ns = (
                timing_info["row_read_ns"] * timing_info["num_read"] +
                timing_info["row_write_ns"] * timing_info["num_write"] +
                timing_info["tccd_ns"] * timing_info["num_logic"]
            )

            # Include exec time in comment for operation
            print("      { PimCmdEnum::%-13s { " % (op.upper() + ','))
            print(f"           {{ Estimated Time, {exec_time_ns:.3f} ns }},")


            for core in sorted(core_data.keys(), key=lambda x: int(x)):
                row_str = ", ".join(["%s:%s" % (idx, count) for idx, count in core_data[core]["row"]])
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
