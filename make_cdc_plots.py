#!/usr/bin/env python3
import sys
import os
import re
import sys
import os
import parser as emup
import matplotlib.pyplot as plt
import numpy as np
import glob
from pathlib import Path

plt.switch_backend('agg')


def save_plot(name):
    print("Generating " + name)
    plt.savefig(name, bbox_inches="tight")
    plt.clf()


path = sys.argv[1]
assert path.endswith(".cdc")
name = path[:-4]

# jgw
emu_cdc_diff_file = os.environ.get('EMU_CDC_DIFF_FILE')
# if the file is specified check that it exists
if emu_cdc_diff_file:
    if not Path(emu_cdc_diff_file).is_file():
        sys.exit('\n\nEMU_CDC_DIFF_FILE does not exist! Unset variable and rerun.\n\n')

# set prev_path and next_path based on options
if emu_cdc_diff_file and Path(emu_cdc_diff_file).is_file():
    # assume file ends in .npy
    prev_path = os.environ.get('EMU_CDC_DIFF_FILE')
    index = prev_path.find('.npy')
    prev_path = prev_path[:index]
    next_path = prev_path + '.next'
else:
    # find the file matching .cdc.N with the largest N
    prev_idx="-1"
    for file in glob.glob(name + '.cdc.*'):
        tmp_idx = re.search(r'\d+', file).group(0)
        if int(tmp_idx) > int(prev_idx):
            prev_idx = tmp_idx

    next_path = name + ".cdc." + str(int(prev_idx)+1)
    if prev_idx == "-1":
        prev_path = ""
    else:
        prev_path = name + ".cdc." + prev_idx
# jgw

cdcdata = emup.load_cdc(path)
# Zero out the diagonal to form the migration map
migration_map = cdcdata["memory_map"].copy()
np.fill_diagonal(migration_map, 0)
# Pull out only the diagonal to form the memory array
local_memops = np.diag(cdcdata["memory_map"])

# jgw
# subtract mm_prev off of migration_map, and write next_path
if os.environ.get('EMU_CDC_DIFF'):
    if prev_path:
        mm_prev = np.load(prev_path + '.npy')
        # subtract off previous migration map
        migration_map = migration_map - mm_prev

    np.save(next_path, migration_map)
# jgw

plt.imshow(migration_map, interpolation='none', cmap="plasma")
plt.colorbar()
plt.grid(False)
plt.title("Migration Map")
plt.ylabel("Source Nodelet")
plt.xlabel("Target Nodelet")
save_plot(name + "_migration_map.png")

plt.bar(range(len(local_memops)), local_memops)
plt.title("Local memory operations")
plt.ylabel("Number of operations")
plt.xlabel("Nodelet")
save_plot(name + "_local_memops.png")

plt.imshow(cdcdata["remotes_map"], interpolation='none', cmap="plasma")
plt.colorbar()
plt.grid(False)
plt.title("Remotes Map")
plt.ylabel("Source Nodelet")
plt.xlabel("Target Nodelet")
save_plot(name + "_remotes_map.png")
