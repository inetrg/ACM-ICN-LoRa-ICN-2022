import time
import os
import sys

import numpy as np
import pandas as pd

NUM_PACKETS=os.environ.get("NUM_PACKETS",10)
PREFIX="/xid"

def gen_poisson(tx_interval, n_packets=NUM_PACKETS):
    timestamps = np.cumsum(np.random.exponential(tx_interval, n_packets))
    d = pd.DataFrame({"timestamp":timestamps})
    d["time_to_next"] = d.diff(periods=-1)["timestamp"].fillna(-1)*-1
    return d

l = gen_poisson(10)

def info(string):
    sys.stderr.write(f"[INFO] {string}\n")

def send_command(cmd):
    sys.stdout.write(cmd)
    sys.stdout.write("\n")
    sys.stdout.flush()

for n,r in l.iterrows():
    cmd = f"ccnl_int {PREFIX}/ccn_data_{n}"
    time_to_next = r["time_to_next"]
    info(f"Next interest: {time_to_next}\n")
    send_command(cmd)
    time.sleep(time_to_next)
