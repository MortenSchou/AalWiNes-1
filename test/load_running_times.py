#!/usr/bin/python
import json
import sys
from os import listdir
from os.path import isfile, join
import math
import copy
from sys import stdout
import csv

print(sys.argv[1:])

bin_hash = sys.argv[1]
test_data = []
test_numbers = []
moped_win = 0
post_win = 0
pre_win = 0
red0_win = 0
red1_win = 0
red2_win = 0
red3_win = 0

bin_hash_split = bin_hash.split("-")

file = open('test_results.csv', 'w')
with file:
    writer = csv.writer(file)
    fnames = ['Network', 'Nodes', 'Labels', 'Rules', 'PDA-States', "PDA-Rules", "States_reduction", "Rules_reduction",
                'Query-Fails', 'Query-Type',
                'Engine', 'Reduction', 'Compilation-time', 'Reduction-time', 'Verification-time', 'Total-time']
    writer = csv.DictWriter(file, fieldnames=fnames)
    writer.writeheader()
    for f in listdir("results/" + bin_hash):
        verification_compilation = []
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
                                                            queries = []
                                                            newFiles = []
                                                            # Modify file to work
                                                            for fi in files:
                                                                line_nr = 0
                                                                writeobject = ""
                                                                for line in fi:
                                                                    if line_nr == 6 or line_nr == 7 or line_nr == 8:
                                                                        line = line + ','
                                                                    writeobject = writeobject + line
                                                                    line_nr = line_nr + 1
                                                                newFiles.append(
                                                                    writeobject)

                                                            for i in range(len(files)):
                                                                # jd = json.load(files[i])
                                                                jd = json.loads(
                                                                    newFiles[i])
                                                                queries.append(
                                                                    jd["answers"]["Q1"])

                                                            # Get universal network stats
                                                            test_numbers = [int(s[1:])
                                                                            for s in f.split('-') if s[1:].isdigit()]
                                                            test_name = [
                                                                s for s in f.split('-')]
                                                            Q = test_numbers[0]
                                                            fails = (int)(math.floor(
                                                                (Q - 1) / 5))
                                                            Q_type = (
                                                                Q - 1) % 5 + 1

                                                            

                                                            reduction = 0
                                                            engine = 0
                                                            for qs in range(len(queries)):
                                                                reduction = reduction + 1
                                                                if qs % 4 == 0:
                                                                    reduction = 0
                                                                    engine = engine + 1
                                                                q = queries[qs]
                                                                verification_compilation.append(
                                                                    {"Engine": engine, "Reduction": reduction, "Total": q['verification-time'] + q['compilation-time'] + q['reduction-time'],
                                                                        "Compilation-time": q["compilation-time"], "Verification-time": q['verification-time'], "Reduction-time": q["reduction-time"]})

                                                                writer.writerow({'Network': test_name[0], 'Nodes': queries[0]['network_node_size'], 'Labels': queries[0]['network_label_size'],
                                                                                 'Rules': queries[0]['network_rules_size'], 'PDA-States': queries[0]['pda_states_rules'][0], 'PDA-Rules': queries[0]['pda_states_rules'][1],
                                                                                 'States_reduction': q['pda_states_rules_reduction'][0], 'Rules_reduction': q['pda_states_rules_reduction'][1],
                                                                                 'Query-Fails': fails, 'Query-Type': Q_type,
                                                                                 'Engine': engine, 'Reduction': reduction, 'Compilation-time': q['compilation-time'],'Reduction-time': q['reduction-time'], 
                                                                                 'Verification-time': q['verification-time'], 'Total-time': q['verification-time'] + q['compilation-time'] + q['reduction-time']})

                                                            sorted_verification_data = sorted(
                                                                verification_compilation, key = lambda k: k['Total'])

                                                            # Find fastest engine for verification + reduction
                                                            if sorted_verification_data[0]['Engine'] == 3:
                                                                pre_win = pre_win + 1
                                                            elif sorted_verification_data[0]['Engine'] == 2:
                                                                post_win = post_win + 1
                                                            elif sorted_verification_data[0]['Engine'] == 1:
                                                                moped_win = moped_win + 1

                                                            if sorted_verification_data[0]['Reduction'] == 0:
                                                                red0_win = red0_win + 1
                                                            elif sorted_verification_data[0]['Reduction'] == 1:
                                                                red1_win = red1_win + 1
                                                            elif sorted_verification_data[0]['Reduction'] == 2:
                                                                red2_win = red2_win + 1
                                                            elif sorted_verification_data[0]['Reduction'] == 3:
                                                                red3_win = red3_win + 1

                                                            test_data.append({"Network": {"Nodes": queries[0]["network_node_size"], "Labels": queries[0]["network_label_size"], "Rules": queries[0]["network_rules_size"],
                                                                                          "States": queries[0]["pda_states_rules"][0], "PDA_Rules": queries[0]["pda_states_rules"][1],
                                                                                          "States_reduction": queries[0]["pda_states_rules_reduction"][0], "Rules_reduction": queries[0]["pda_states_rules_reduction"][1]},
                                                                              "Query": {"Failover": (int)(fails), "Type": Q_type},
                                                                              "Verification": tuple(sorted_verification_data)})

                                                        except json.decoder.JSONDecodeError as e:
                                                            print(f)
                                                            print(e)

for t in test_data:
    print(t)

print("Moped win:" + str(moped_win))
print("Post* win:" + str(post_win))
print("Pre* win:" + str(pre_win))

print("Reduction0 win:" + str(moped_win))
print("Reduction1 win:" + str(post_win))
print("Reduction2 win:" + str(pre_win))
print("Reduction3 win:" + str(pre_win))


# print("\nLatex Table Rows")
for t in test_data[:10]:
    print(str(t['Network']['Nodes']) + "&" + str(t['Network']['Rules']) + ";" + str(t['Network']['Labels']) + "&" +
          str(t['Query']['Type']) + ";" + str(t['Query']['Failover']) + "&" +
          str(t['Verification'][0]['Engine']) + "&" + str(t['Verification'][0]['Reduction']) + "&" + str(round(t['Verification'][0]['Verification-time'], 3)) + "&" + str(round(t['Verification'][0]['Total'], 3)) +
          "\\\ \hline")

for t in test_data[len(test_data)-10:]:
    print(str(t['Network']['Nodes']) + "&" + str(t['Network']['Rules']) + ";" + str(t['Network']['Labels']) + "&" +
          str(t['Query']['Type']) + ";" + str(t['Query']['Failover']) + "&" +
          str(t['Verification'][0]['Engine']) + "&" + str(t['Verification'][0]['Reduction']) + "&" + str(round(t['Verification'][0]['Verification-time'], 3)) + "&" + str(round(t['Verification'][0]['Total'], 3)) +
          "\\\ \hline")

# print("\nLatex Table Rows ALL")
# for t in sorted_data:
# print(t['Name'].replace('_', '\_')[6:] + "&" + str(round(t['Compilation-time'], 3)) + "&" + str(round(t['Compilation ration'], 3)) + "&" + str(round(t['Vtime Moped'], 3)) + "&" + str(round(t['Vtime Post*'], 3)) + "&" + str(round(t['Ratio(E1/E2)'], 3)) + "\\\ \hline")
