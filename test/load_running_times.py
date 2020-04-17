import json
import sys
from os import listdir
from os.path import isfile, join
import math
import copy
from sys import stdout
import pandas as pd

print(sys.argv[1:])

bin_hash = sys.argv[1]
test_data = []
verification_compilation = []
test_numbers = []
moped_win = 0
post_win = 0
pre_win = 0
bin_hash_split = bin_hash.split("-")

for f in listdir("results/" + bin_hash):
    with open("results/" + bin_hash + "/" + f) as of1:
        with open("results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E1-R1/" + f) as of1r1:
            with open("results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E1-R2/" + f) as of1r2:
                with open("results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E1-R3/" + f) as of1r3:
                    with open("results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E2-" + bin_hash_split[3] + "/" + f) as of2:
                        with open("results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E2-R1/" + f) as of2r1:
                            with open("results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E2-R2/" + f) as of2r2:
                                with open("results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E2-R3/" + f) as of2r3:
                                    with open("results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E3-" + bin_hash_split[3] + "/" + f) as of3:
                                        with open("results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E3-R1/" + f) as of3r1:
                                            with open("results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E3-R2/" + f) as of3r2:
                                                with open("results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E3-R3/" + f) as of3r3:
                                                    try:
                                                        files = [
                                                            of1, of1r1, of1r2, of1r3, of2, of2r1, of2r2, of2r3, of3, of3r1, of3r2, of3r3]
                                                        queries = [
                                                            [] for x in range(files)]
                                                        for i in range(files):
                                                            jd = json.load(
                                                                files[i])
                                                            for q in jd["answers"]:
                                                                queries[i].append(
                                                                    q)

                                                        # Get universal network stats
                                                        network_stats = {
                                                            "Nodes": queries[0][0]["network_node_size"], "Labels": queries[0][0]["network_label_size"], "Rules": queries[0][0]["network_rules_size"]}
                                                        pda_stats = {
                                                            "States": queries[0][0]["pda_states_rules"][0], "Rules": queries[0][0]["pda_states_rules"][1]}
                                                        pda_stats_after = {
                                                            "States_reduction": queries[0][0]["pda_states_rules_reduction"][0], "Rules_reduction": queries[0][0]["pda_states_rules_reduction"][1]}
                                                        test_numbers = [int(s[1:])
                                                                        for s in f.split('_') if s[1:].isdigit()]

                                                        reduction = 0
                                                        engine = 0
                                                        for qs in range(queries):
                                                            qn = 0
                                                            reduction = reduction + 1
                                                            if qs % 4:
                                                                reduction = 0
                                                                engine = engine + 1

                                                            for q in queries[qs]:
                                                                qn = qn + 1
                                                                fails = 0
                                                                # Adapt to Morten structure
                                                                if qn > 3:
                                                                    fails = 1
                                                                verification_compilation.append(
                                                                    {"Engine": engine, "Reduction": reduction, "Failover": fails, "Compilation_verification": q['verification-time'] + q['compilation-time'] + q['reduction-time'],
                                                                     "Compilation-time": q["compilation-time"], "Verification-time": q['verification-time'], "Reduction-time": q["reduction-time"]})

                                                        sorted_verification_data = sorted(
                                                            verification_compilation, key=lambda k: k['Compilation_verification'])

                                                        # Find fastest engine for verification + reduction
                                                        if sorted_verification_data[0]['Index'] < 7:
                                                            pre_win = pre_win + 1
                                                        elif 3 < sorted_verification_data[0]['Index'] >= 7:
                                                            post_win = post_win + 1
                                                        else:
                                                            moped_win = moped_win + 1

                                                        test_data.append(
                                                            {"Network": {test_numbers, network_stats, pda_stats, pda_stats_after}, "Test": test_numbers, "Data": sorted_verification_data})

                                                    except json.decoder.JSONDecodeError as e:
                                                        print(f)
                                                        print(e)

for t in test_data:
    print(t)

print("Moped win:" + str(moped_win))
print("Post* win:" + str(post_win))
print("Pre* win:" + str(pre_win))

df = pd.read_json(test_data)
exportcsv = df.to_csv(r'./test_data.csv', index=None)

#print("\nLatex Table Rows")
# for t in sorted_data[:10]:
#print(t['Name'].replace('_', '\_')[6:] + "&" + str(round(t['Compilation-time'], 3)) + "&" + str(round(t['Compilation ration'], 3)) + "&" + str(round(t['Vtime Moped'], 3)) + "&" + str(round(t['Vtime Post*'], 3)) + "&" + str(round(t['Ratio(E1/E2)'], 3)) + "\\\ \hline")

#print("\nLatex Table Rows ALL")
# for t in sorted_data:
#print(t['Name'].replace('_', '\_')[6:] + "&" + str(round(t['Compilation-time'], 3)) + "&" + str(round(t['Compilation ration'], 3)) + "&" + str(round(t['Vtime Moped'], 3)) + "&" + str(round(t['Vtime Post*'], 3)) + "&" + str(round(t['Ratio(E1/E2)'], 3)) + "\\\ \hline")
