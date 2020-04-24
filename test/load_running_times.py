#!/usr/bin/python
import json
import sys
from os import listdir
from os.path import isfile, join
import math
import copy
from sys import stdout
import csv
from pathlib import Path
from os import walk

print(sys.argv[1:])

bin_hash = sys.argv[1]
test_data = []
reductions = []
test_numbers = []
moped_win = 0
post_win = 0
pre_win = 0
red0_win = 0
red1_win = 0
red2_win = 0
red3_win = 0
engine_ratio = []

#Moped vs post
moped_post = [[0 for s in range(4)] for i in range(4)]
#Moped vs pre
moped_pre = [[0 for s in range(4)] for i in range(4)]
post_pre = [[0 for s in range(4)] for i in range(4)]

bin_hash_split = bin_hash.split("-")

file = open('test_results.csv', 'w')
file_all = open('test_results_all.csv', 'w')
with file:
    writer = csv.writer(file)
    writer_all = csv.writer(file_all)
    fnames = ['Network', 'Size-factor', 'Nodes', 'Labels', 'Rules', 'PDA-States', "PDA-Rules", "States_reduction", "Rules_reduction",
              'Query-Fails', 'Query-Type',
              'Engine', 'Reduction', 'Compilation-time', 'Reduction-time', 'Verification-time', 'Total-time']
    writer = csv.DictWriter(file, fieldnames=fnames)
    writer_all = csv.DictWriter(file_all, fieldnames=fnames)
    writer.writeheader()
    writer_all.writeheader()

    tests =[]
    for t in ['Aarnet', 'Agis', 'Bandcon', 'Bellcanada', 'Claranet', 'DeutscheTelekom']:
        for N in range(1,6):
            for Q in range(1,6):
                for k in range(0,4):
                    tests.append(t + "-" + str(N) + "-Q" + str(Q) + "-k" + str(k))
    for f in tests:
        files = []
        verification_compilation = []
        myfile = Path("results/" + bin_hash + "/" + f)
        if myfile.is_file():
            files.append(myfile)
        myfile = Path(
            "results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E1-R1/" + f)
        if myfile.is_file():
            files.append(myfile)
        myfile = Path(
            "results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E1-R2/" + f)
        if myfile.is_file():
            files.append(myfile)
        myfile = Path(
            "results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E1-R3/" + f)
        if myfile.is_file():
            files.append(myfile)
        myfile = Path("results/" + bin_hash_split[0] + "-" +
                      bin_hash_split[1] + "-E2-" + bin_hash_split[3] + "/" + f)
        if myfile.is_file():
            files.append(myfile)
        myfile = Path(
            "results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E2-R1/" + f)
        if myfile.is_file():
            files.append(myfile)
        myfile = Path(
            "results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E2-R2/" + f)
        if myfile.is_file():
            files.append(myfile)
        myfile = Path(
            "results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E2-R3/" + f)
        if myfile.is_file():
            files.append(myfile)
        myfile = Path("results/" + bin_hash_split[0] + "-" +
                      bin_hash_split[1] + "-E3-" + bin_hash_split[3] + "/" + f)
        if myfile.is_file():
            files.append(myfile)
        myfile = Path(
            "results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E3-R1/" + f)
        if myfile.is_file():
            files.append(myfile)
        myfile = Path(
            "results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E3-R2/" + f)
        if myfile.is_file():
            files.append(myfile)
        myfile = Path(
            "results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-E3-R3/" + f)
        if myfile.is_file():
            files.append(myfile)
        try:
            if not files:
                continue
            queries = []

            for i in range(len(files)):
                jd = json.load(open(files[i]))
                name = files[i].parts[1]
                test_type = [s for s in name.split('-') if s[1:].isdigit()]
                engine = test_type[0]
                reduction = test_type[1]
                queries.append({"Name": name, "Reduction": reduction, "Engine": engine, "Data": jd["answers"]["Q1"]})

            # Get universal network stats
            test_numbers = [int(s[1:])
                            for s in f.split('-') if s[1:].isdigit()]
            test_name = [s for s in f.split('-')]
            Q_type = test_numbers[0]
            Q_fails = test_numbers[1]

            for q in queries:
                engine = q['Engine']
                reduction = q['Reduction']
                q = q['Data']
                verification_compilation.append(
                    {"Engine": engine, "Reduction": reduction, "Total": q['verification-time'] + q['compilation-time'] + q['reduction-time'],
                        "Compilation-time": q["compilation-time"], "Verification-time": q['verification-time'], "Reduction-time": q["reduction-time"], "Output": q['result']})
                
                if len([s for s in queries if s['Engine'] == engine]) == 4:
                    writer.writerow({'Network': test_name[0], 'Size-factor': test_name[1], 'Nodes': q['network_node_size'], 'Labels': q['network_label_size'],
                                'Rules': q['network_rules_size'], 'PDA-States': q['pda_states_rules'][0], 'PDA-Rules': q['pda_states_rules'][1],
                                'States_reduction': q['pda_states_rules_reduction'][0], 'Rules_reduction': q['pda_states_rules_reduction'][1],
                                'Query-Fails': Q_fails, 'Query-Type': Q_type,
                                'Engine': engine, 'Reduction': reduction, 'Compilation-time': q['compilation-time'], 'Reduction-time': q['reduction-time'],
                                'Verification-time': q['verification-time'], 'Total-time': q['verification-time'] + q['compilation-time'] + q['reduction-time']})
                    if engine == "E2":
                        reductions.append({ 'Network': test_name[0], 'PDA-States': q['pda_states_rules'][0], 'PDA-Rules': q['pda_states_rules'][1],
                                            'States_reduction': q['pda_states_rules_reduction'][0], 'Rules_reduction': q['pda_states_rules_reduction'][1], 
                                            "R": reduction, "Vtime": q['verification-time'] + q['reduction-time']})

                writer_all.writerow({'Network': test_name[0], 'Size-factor': test_name[1], 'Nodes': q['network_node_size'], 'Labels': q['network_label_size'],
                                'Rules': q['network_rules_size'], 'PDA-States': q['pda_states_rules'][0], 'PDA-Rules': q['pda_states_rules'][1],
                                'States_reduction': q['pda_states_rules_reduction'][0], 'Rules_reduction': q['pda_states_rules_reduction'][1],
                                'Query-Fails': Q_fails, 'Query-Type': Q_type,
                                'Engine': engine, 'Reduction': reduction, 'Compilation-time': q['compilation-time'], 'Reduction-time': q['reduction-time'],
                                'Verification-time': q['verification-time'], 'Total-time': q['verification-time'] + q['compilation-time'] + q['reduction-time']})

            sorted_verification_data = sorted(
                verification_compilation, key=lambda k: k['Verification-time'])

            Percentage = 0.5
            Second = 0.001
            # Find fastest engine for verification + reduction


            i = -1
            for r in ["R0", "R1", "R2", "R3"]:
                i = i + 1
                list_post_moped = [s for s in sorted_verification_data if s['Reduction'] == r and s['Engine'] != 'E3']
                if len(list_post_moped) == 2:
                    #Even
                    if (list_post_moped[1]['Verification-time'] - list_post_moped[0]['Verification-time']) / list_post_moped[1]['Verification-time'] < Percentage or list_post_moped[1]['Verification-time'] - list_post_moped[0]['Verification-time'] < Second:
                        moped_post[2][i] = moped_post[2][i] + 1
                    elif list_post_moped[0]['Engine'] == 'E1':
                        moped_post[0][i] = moped_post[0][i] + 1
                    elif list_post_moped[0]['Engine'] == 'E2':
                        moped_post[1][i] = moped_post[1][i] + 1
                else:
                    moped_post[3][i] = moped_post[3][i] + 1

                list_pre_moped = [s for s in sorted_verification_data if s['Reduction'] == r and s['Engine'] != 'E2']
                if len(list_pre_moped) == 2: 
                    #Even
                    if (list_pre_moped[1]['Verification-time'] - list_pre_moped[0]['Verification-time']) / list_pre_moped[1]['Verification-time'] < Percentage or list_pre_moped[1]['Verification-time'] - list_pre_moped[0]['Verification-time'] < Second:
                        moped_pre[2][i] = moped_pre[2][i] + 1
                    elif list_pre_moped[0]['Engine'] == 'E1':
                        moped_pre[0][i] = moped_pre[0][i] + 1
                    elif list_pre_moped[0]['Engine'] == 'E3':
                        moped_pre[1][i] = moped_pre[1][i] + 1
                else:
                    moped_pre[3][i] = moped_pre[3][i] + 1

                list_post_pre = [s for s in sorted_verification_data if s['Reduction'] == r and s['Engine'] != 'E1']
                if len(list_post_pre) == 2: 
                    #Even
                    if (list_post_pre[1]['Verification-time'] - list_post_pre[0]['Verification-time']) / list_post_pre[1]['Verification-time'] < Percentage or list_post_pre[1]['Verification-time'] - list_post_pre[0]['Verification-time'] < Second:
                        post_pre[2][i] = post_pre[2][i] + 1
                    elif list_post_pre[0]['Engine'] == 'E2':
                        post_pre[0][i] = post_pre[0][i] + 1
                    elif list_post_pre[0]['Engine'] == 'E3':
                        post_pre[1][i] = post_pre[1][i] + 1
                else:
                    post_pre[3][i] = post_pre[3][i] + 1

            test_data.append({"Network": {"Nodes": queries[0]['Data']["network_node_size"], "Labels": queries[0]['Data']["network_label_size"], "Rules": queries[0]['Data']["network_rules_size"],
                                        "States": queries[0]['Data']["pda_states_rules"][0], "PDA_Rules": queries[0]['Data']["pda_states_rules"][1],
                                        "States_reduction": queries[0]['Data']["pda_states_rules_reduction"][0], "Rules_reduction": queries[0]['Data']["pda_states_rules_reduction"][1]},
                            "Query": {"Failover": (int)(Q_fails), "Type": Q_type},
                            "Verification": tuple(sorted_verification_data)})

        except json.decoder.JSONDecodeError as e:
            print(f)
            print(e)


sorted_test_data = sorted(test_data, key=lambda k: k['Verification'][0]['Total'])

for t in sorted_test_data:
    print(t)


#Print fastest overall time
print("MopedPost")
print(moped_post)

i = -1
for E in moped_post:
    i = i + 1
    if i == 0:
        engine = "Moped"
    elif i == 1:
        engine = "Post"
    elif i == 2:
        engine = "Even"
    elif i == 3:
        engine = "Unknown"
    print(engine + "&" + str(E[0]) + "&" + str(E[1]) + "&" + str(E[2]) + "&" + str(E[3]) + "\\\ \hline")

print("moped_pre")
print(moped_pre)
i = -1
for E in moped_pre:
    i = i + 1
    if i == 0:
        engine = "Moped"
    elif i == 1:
        engine = "Pre"
    elif i == 2:
        engine = "Even"
    elif i == 3:
        engine = "Unknown"
    print(engine + "&" + str(E[0]) + "&" + str(E[1]) + "&" + str(E[2]) + "&" + str(E[3]) + "\\\ \hline")

print("post_pre")
print(post_pre)
i = -1
for E in post_pre:
    i = i + 1
    if i == 0:
        engine = "Post"
    elif i == 1:
        engine = "Pre"
    elif i == 2:
        engine = "Even"
    elif i == 3:
        engine = "Unknown"
    print(engine + "&" + str(E[0]) + "&" + str(E[1]) + "&" + str(E[2]) + "&" + str(E[3]) + "\\\ \hline")

print("Reduction0 win:" + str(red0_win))
print("Reduction1 win:" + str(red1_win))
print("Reduction2 win:" + str(red2_win))
print("Reduction3 win:" + str(red3_win))

#Latex print fastest reduction on fastest engine = 2 :)
bufferNetworks = []
buffer = {"Network": reductions[0]['Network'], "R0": 0, "R1": 0, "R2": 0, "R3": 0}
try:
    print("Fastest reduction")
    # Reductions
    for r in reductions:
        network = str(r['Network'])
        reduction = str(r['R'])
        if network != buffer['Network']:
            bufferNetworks.append({"Network": buffer['Network'], "R0": buffer['R0'], "R1": buffer['R1'], "R2": buffer['R2'], "R3": buffer['R3']})
            nets = [net for net in bufferNetworks if net['Network'] == network]
            if len(nets) == 1:
                buffer = bufferNetworks.pop(bufferNetworks.index(nets[0]))
            else:
                buffer = {"Network": network, "R0": 0, "R1": 0, "R2": 0, "R3": 0}
        buffer[reduction] = (buffer[reduction] + r['Vtime']) / 2

except BaseException as e:
    print(e)

# print("\nLatex Table Rows")
bufferNetworks.append({"Network": buffer['Network'], "R0": buffer['R0'], "R1": buffer['R1'], "R2": buffer['R2'], "R3": buffer['R3']})
for buffer in bufferNetworks:
    if buffer['R3'] != 0:
        speedup = ((buffer['R0'] - buffer['R3'])/buffer['R3'])
        print(buffer['Network'] + "&" + str(round(buffer['R0'],3)) + "&" + str(round(buffer['R1'],3)) + "&" + str(round(buffer['R2'],3)) + "&" + str(round(buffer['R3'],3))+ "&" + str(round(speedup,3)) + "\\\ \hline")

#Latex print fastest engine on fastest reducktion = 0
def get_ratio(engineA, engineB):
    engine_ratio = []
    engine_ratio_false = []

    for t in test_data:
        i = -1
        
        for r in ["R0", "R1", "R2", "R3"]:
            try:
                first = next(s for s in t['Verification'] if s['Engine'] == engineA and s['Reduction'] == r)
                fastest_time = first['Verification-time']
                next_veri = next(s for s in t['Verification'] if s['Engine'] == engineB and s['Reduction'] == r)
                #Remove very fast cases on both engines
                if  next_veri['Verification-time'] < 0.5 and fastest_time < 0.5: continue
                if  abs(next_veri['Verification-time'] - fastest_time) < 0.001: continue
            except:
                continue
            try:
                ratio =  fastest_time / next_veri['Verification-time']
                #Get and print Output result
                if first['Output'] == True:
                    engine_ratio.append({"Test": t, "FT": fastest_time, "R": r, "NT": next_veri['Verification-time'], "NE": next_veri['Engine'], "Ratio": ratio})
                else:
                    engine_ratio_false.append({"Test": t, "FT": fastest_time, "R": r, "NT": next_veri['Verification-time'], "NE": next_veri['Engine'], "Ratio": ratio})
            except:
                print("some")
                    
    def printline(list, bi, ei):
        sorted_engine_ratio = sorted(list, key=lambda k: k['Ratio'])
        for t in sorted_engine_ratio[bi:ei]:
            print(str(t['Test']['Network']['Nodes']) + "&" +  str(t['Test']['Network']['Rules']) + "&" + str(t['Test']['Network']['Labels']) 
                + "&" + str(t['Test']['Network']['States_reduction']) + "&" + str(t['Test']['Network']['Rules_reduction']) + "&" + str(t['Test']['Query']) + "&" + str(t['R'])
                + "&" + str(round(t['FT'], 4)) + "&" + str(round(t['NT'], 4)) + "&" + str(round(t['Ratio'], 4)) + "\\\ \hline")
        print("$\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$\\\ \hline")


    print("True")

    print(engineA + engineB)
    print("\\begin{table}[H]\n\\centering\n\\begin{adjustwidth}{-2cm}{}\n\\begin{tabular}{|c|c|c|c|c|c|c|c|c|c|}\n \hline \n \\rowcolor[HTML]{9B9B9B}\n \\textbf{Nodes}& \\textbf{Rules} & \\textbf{Labels} & \\textbf{PDA-S} & \\textbf{PDA-R} & \\textbf{Query} & \\textbf{R} & \\textbf{V1} & \\textbf{V2} & \\textbf{Ratio}\\\ \hline")

    printline(engine_ratio, 0, 5)
    mid = int(len(engine_ratio)/2)
    printline(engine_ratio, mid, mid + 5)
    last = int(len(engine_ratio))
    printline(engine_ratio, last - 10, last)

    print("False")

    print("\\begin{table}[H]\n\\centering\n\\begin{adjustwidth}{-2cm}{}\n\\begin{tabular}{|c|c|c|c|c|c|c|c|c|c|}\n \hline \n \\rowcolor[HTML]{9B9B9B}\n \\textbf{Nodes}& \\textbf{Rules} & \\textbf{Labels} & \\textbf{PDA-S} & \\textbf{PDA-R} & \\textbf{Query} & \\textbf{R} & \\textbf{V1} & \\textbf{V2} & \\textbf{Ratio}\\\ \hline")
    printline(engine_ratio_false, 0, 5)
    mid = int(len(engine_ratio_false)/2)
    printline(engine_ratio_false, mid, mid + 5)
    last = int(len(engine_ratio_false))
    printline(engine_ratio_false, last - 10, last)


get_ratio("E1","E2")
get_ratio("E1","E3")
get_ratio("E2","E3")