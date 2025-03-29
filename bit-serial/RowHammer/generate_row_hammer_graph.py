#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import pandas as pd
import matplotlib.pyplot as plt
import sys

# Load the data
file_path = sys.argv[1] # Make sure this file is in the same directory
df = pd.read_csv(file_path, delim_whitespace=True)

# Thresholds to evaluate
thresholds = [2000, 2222, 4000]

# Determine color for each bar
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

# Apply color mapping
bar_colors = df["Accesses_per_RI"].apply(get_bar_color)

# Create the plot
plt.figure(figsize=(14, 6))
plt.bar(df["Command"], df["Accesses_per_RI"], color=bar_colors)

# Add plot labels and title
plt.xlabel("PIM op Command")
plt.ylabel("Accesses per Row Interval")
plt.xticks(rotation=90)
plt.grid(axis='y')

# Add horizontal reference lines
plt.axhline(y=2000, color='red', linestyle='--', label='y = 2000')
plt.axhline(y=2222, color='green', linestyle='--', label='y = 2222')
plt.axhline(y=4000, color='blue', linestyle='--', label='y = 4000')

# Add legend
plt.legend()

# Improve layout
plt.tight_layout()

# Save as PDF
plt.savefig("Accesses_per_RI_Histogram_colored.pdf", format="pdf")
print("Saved: Accesses_per_RI_Histogram_colored.pdf")
