import json
import sys
from os import listdir
from os.path import isfile, join
import math
import copy
from sys import stdout

print(sys.argv[1:])

bin_hash = sys.argv[1]

test_data = []
rtime = []
btime = []
vtime = []
redstat = []
reds = []

test_numbers = []

bin_hash_split = bin_hash.split("-")

for f in listdir("results/" + bin_hash):
    with open("results/" + bin_hash + "/" + f) as of1:
        with open(
                "results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E2-" + bin_hash_split[3] + "/" + f) as of2:
            try:
                jd1 = json.load(of1)
                jd2 = json.load(of2)
                T = jd1["answers"]["Q1"]
                T2 = jd2["answers"]["Q1"]
                rtime.append(T["reduction-time"])
                btime.append(T["compilation-time"])
                vtime.append(T["verification-time"])
                redstat.append(T["reduction"][1]/T["reduction"][0])
                reds.append(T["reduction"][1])
                test_numbers = [int(s[1:]) for s in f.split('_') if s[1:].isdigit()]
                test_data.append({"Name": f, "Vtime Moped": T["verification-time"], "Vtime Post*": T2["verification-time"], "Ratio(E1/E2)": T["verification-time"]/T2["verification-time"]})
            except json.decoder.JSONDecodeError as e:
                ""

sorted_data = sorted(test_data, key=lambda k: k['Ratio(E1/E2)'], reverse=True)

for t in sorted_data:
    print(t)