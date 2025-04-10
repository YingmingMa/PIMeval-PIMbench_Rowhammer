#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Load the data from text files
file_8 = "mostAccessperRI8.txt"
file_16 = "mostAccessperRI16.txt"
file_32 = "mostAccessperRI32.txt"

# Read the files into DataFrames
df_8 = pd.read_csv(file_8, delim_whitespace=True)
df_16 = pd.read_csv(file_16, delim_whitespace=True)
df_32 = pd.read_csv(file_32, delim_whitespace=True)

# Add a column to identify bit width
df_8["Bit_Width"] = "8-bit"
df_16["Bit_Width"] = "16-bit"
df_32["Bit_Width"] = "32-bit"

# Combine all data into one DataFrame
df_all = pd.concat([df_8, df_16, df_32])

# Pivot data for plotting
df_pivot = df_all.pivot(index="Command", columns="Bit_Width", values="Accesses_per_ms").fillna(0)

# Reorder columns to be 32-bit, 16-bit, 8-bit
df_pivot = df_pivot[["32-bit", "16-bit", "8-bit"]]

# Plotting
plt.figure(figsize=(20, 10))
colors = sns.color_palette("muted")  # Muted color palette
ax = df_pivot.plot(kind='bar', color=colors, figsize=(20, 10))

# Add reference lines
for y in [2000, 2222, 4000]:
    plt.axhline(y=y, color='gray', linestyle='--', linewidth=1)
    plt.text(len(df_pivot)-0.5, y + 50, f'{y} accesses/ms', color='gray')

# Title and labels
plt.title("Accesses per ms by Command and Bit Width (Ordered 32, 16, 8)")
plt.xlabel("PIM Command")
plt.ylabel("Accesses per ms")
plt.xticks(rotation=90)
plt.grid(axis='y')
plt.tight_layout()

# Save to PDF
plt.savefig("Accesses_per_ms_by_Command.pdf", format="pdf")
