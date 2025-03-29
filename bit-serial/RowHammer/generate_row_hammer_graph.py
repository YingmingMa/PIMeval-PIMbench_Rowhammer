#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import pandas as pd
import matplotlib.pyplot as plt
import sys

# Load the data file passed as command-line argument
file_path = sys.argv[1]
df = pd.read_csv(file_path, delim_whitespace=True)

# Define thresholds for coloring and lines
thresholds = [2000, 2222, 4000]

# Determine color for each bar based on how many thresholds it passes
def get_bar_color(value):
    passed = sum(value > t for t in thresholds)
    if passed == 3:
        return 'darkblue'
    elif passed == 2:
        return 'dodgerblue'
    elif passed == 1:
        return 'skyblue'
    else:
        return 'lightgray'

# Apply color mapping to each command
bar_colors = df["Accesses_per_ms"].apply(get_bar_color)

# Create the bar plot
plt.figure(figsize=(14, 6))
plt.bar(df["Command"], df["Accesses_per_ms"], color=bar_colors)

# Add labels and title
plt.xlabel("PIM Command")
plt.ylabel("Accesses per ms")
plt.xticks(rotation=90)
plt.grid(axis='y')

# # Add horizontal reference lines
# plt.axhline(y=2000, color='red', linestyle='--', label='2000 accesses/ms')
# plt.axhline(y=2222, color='green', linestyle='--', label='2222 accesses/ms')
# plt.axhline(y=4000, color='blue', linestyle='--', label='4000 accesses/ms')

# # Add legend
# plt.legend()

# Layout adjustment
plt.tight_layout()

# Save to PDF
plt.savefig("Accesses_per_ms_Histogram_colored.pdf", format="pdf")
print("Saved: Accesses_per_ms_Histogram_colored.pdf")
