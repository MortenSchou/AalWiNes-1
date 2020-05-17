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
import statistics

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
engine_ratio = []
min_dif_veri = []

#Moped vs post
mpost_post = [[0 for s in range(4)] for i in range(4)]
#Moped vs pre
mpre_pre = [[0 for s in range(4)] for i in range(4)]
mpost_mpre = [[0 for s in range(4)] for i in range(4)]
post_pre = [[0 for s in range(4)] for i in range(4)]

mipost_post = [[0 for s in range(4)] for i in range(4)]
mipre_pre = [[0 for s in range(4)] for i in range(4)]

post_r = [[0 for s in range(4)] for i in range(7)]

bin_hash_split = bin_hash.split("-")

def manipulateFile(file):
    writer = ""
    line_nr = 0
    for line in open(file):
        if line_nr == 6 or line_nr == 7:
            emptyline = ""
            for char in line:
                if char == ":":
                    char = char + "\""
                if char == ",":
                    char = "\"" + char
                emptyline = emptyline + char
            line = emptyline
        writer = writer + line
        line_nr = line_nr + 1
    return writer


def Average(lst): 
    return sum(lst) / len(lst) 

def min_dif(lst):
    lst2 = lst[10:20]
    lst1 = lst[0:10]
    return min([abs(x - y) for x, y in zip(lst1, lst2)])

def find_win(enginea, engineb, reduction, con_win, Percentage, Second, sorted_verification_data):
    list_win = [s for s in sorted_verification_data if s['Reduction'] == reduction and (s['Engine'] == enginea or s['Engine'] == engineb)]
    i = int(reduction[1:])
    if len(list_win) == 2:
        #Even
        if (list_win[1]['Verification-time'] - list_win[0]['Verification-time']) / list_win[1]['Verification-time'] < Percentage or list_win[1]['Verification-time'] - list_win[0]['Verification-time'] < Second:
            con_win[2][i] = con_win[2][i] + 1
        elif list_win[0]['Engine'] == enginea:
            con_win[0][i] = con_win[0][i] + 1
        elif list_win[0]['Engine'] == engineb:
            con_win[1][i] = con_win[1][i] + 1
    else:
        con_win[3][i] = con_win[3][i] + 1

def find_reduction_win(engine, con_win, Percentage, Second, sorted_data):
    sorted_data = [s for s in sorted_data if s['Reduction'] != "R0"]
    if len(sorted_data) == 4:
        i = int(sorted_data[0]['Reduction'][1:])
        i1 = int(sorted_data[1]['Reduction'][1:])
        i2 = int(sorted_data[2]['Reduction'][1:])
        #Even
        if (sorted_data[1]['Total'] - sorted_data[0]['Total']) / sorted_data[1]['Total'] < Percentage or sorted_data[1]['Total'] - sorted_data[0]['Total'] < Second:
            con_win[5][1] = con_win[5][1] + 1
            con_win[5][2] = con_win[5][2] + 1
            con_win[5][3] = con_win[5][3] + 1
        else:
            con_win[i][1] = con_win[i][1] + 1
            con_win[i1][2] = con_win[i1][2] + 1
            con_win[i2][3] = con_win[i2][3] + 1
    else:
        con_win[6][1] = con_win[6][1] + 1

finished_test_cases = 0
failed_test_cases = 0
tests =[]
for t in ['Aarnet', 'Agis', 'Bandcon', 'Bellcanada', 'Claranet', 'DeutscheTelekom']:
    for N in range(1,6):
        for k in range(0,4):
            for Q in range(1, 6):
                for QI in range(0, 10):
                    tests.append(t + "-" + str(N) + "-k" + str(k) + "-" + str(Q) + "-" + str(QI))

for f in tests:
    files = []
    MI_Files = []
    verification_compilation = []
    mi_post_veri = []
    for i in range(1,11):
        myfile = Path("results/" + bin_hash + "/E2-R0/" + f + "-" + str(i))
        if myfile.is_file():
            files.append(myfile)

        myfile = Path("results/" + bin_hash + "/E2-R1/" + f + "-" + str(i))
        if myfile.is_file():
            files.append(myfile)
        
        myfile = Path("results/" + bin_hash + "/E2-R2/" + f + "-" + str(i))
        if myfile.is_file():
            files.append(myfile)
        
        myfile = Path("results/" + bin_hash + "/E2-R3/" + f + "-" + str(i))
        if myfile.is_file():
            files.append(myfile)
        
        myfile = Path("results/" + bin_hash + "/E2-R4/" + f + "-" + str(i))
        if myfile.is_file():
            files.append(myfile)

    try:
        if not files:
            continue
        queries = []

        for fi in files:
            try:
                jd = json.load(open(fi))
            except:
                new_fi = manipulateFile(fi)
                jd = json.loads(new_fi)
            name = fi.parts[2]
            test_type = [s for s in name.split('-')]
            test_name = [s for s in fi.name.split('-')]
            engine = test_type[0]
            reduction = test_type[1]
            queries.append({"Name": name, "Repetation": test_name[5], "Reduction": reduction, "Engine": engine, "Data": jd["answers"]["Q1"], "MI": 0})

        # Get universal network stats
        finished_test_cases = finished_test_cases + 1
        test_numbers = [s for s in f.split('-')]
        Q_type = test_numbers[3]
        Q_instance = test_numbers[4]
        Q_fails = test_numbers[2][1:]
        verification_times = []

        for q in queries:
            engine = q['Engine']
            reduction = q['Reduction']
            mi = q['MI']
            rep = q['Repetation']
            q = q['Data']
            t = next((s for s in verification_times if s['Engine'] == engine and s['Reduction'] == reduction and s['MI'] == mi), False)
            if t:
                red_list = t['Reduction-time']
                red_list.append(q['reduction-time'])
                t['Reduction'] = red_list
                veri_list = t['Verification-time']
                veri_list.append(q['verification-time'])
                t['Verification-time'] = veri_list
                com_list = t['Compilation']
                com_list.append(q['compilation-time'])
                t['Compilation'] = com_list
                total_list = t['Total']
                total_list.append(q['verification-time'] + q['reduction-time'])
                t['Total'] = total_list
                if rep == "10":
                    #verification_compilation.append({"MI": mi, "Engine": engine, "Reduction": reduction, "Total": Average(total_time),
                    #        "Compilation-time": Average(compilation_time), "Verification-time": Average(verification_time), "Reduction-time": Average(reduction_time), "Output": q['result']})
                    verification_compilation.append({"MI": mi, "Engine": engine, "Reduction": reduction, "Total": statistics.median(total_list),
                            "Compilation-time": statistics.median(com_list), "Verification-time": statistics.median(veri_list), "Reduction-time": statistics.median(red_list), "Output": q['result']})
            else:
                red_list = []
                veri_list = []
                com_list = []
                total_list = []
                red_list.append(q['reduction-time'])
                veri_list.append(q['verification-time'])
                com_list.append(q["compilation-time"])
                total_list.append(q['verification-time'] + q['compilation-time'] + q['reduction-time'])
                verification_times.append({"MI": mi, "Engine": engine, "Reduction": reduction, "Reduction-time": red_list, "Verification-time": veri_list, "Compilation": com_list, "Total": total_list})


        sorted_verification_data = sorted(
            verification_compilation, key=lambda k: k['Verification-time'])

        Percentage = 30/100
        Second = 0.001
        sorted_reduction_data = sorted(verification_compilation, key=lambda k: k['Total'])
        find_reduction_win("E2", post_r, 10/100, Second, sorted_reduction_data)

        try:
            test_data.append({"Network": {"Nodes": queries[0]['Data']["network_node_size"], "Labels": queries[0]['Data']["network_label_size"], "Rules": queries[0]['Data']["network_rules_size"],
                                "States": queries[0]['Data']["pda_states_rules"][0], "PDA_Rules": queries[0]['Data']["pda_states_rules"][1],
                                "States_reduction": queries[0]['Data']["pda_states_rules_reduction"][0], "Rules_reduction": queries[0]['Data']["pda_states_rules_reduction"][1]},
                    "Query": {"Failover": (int)(Q_fails), "Type": Q_type},
                    "Verification": tuple(sorted_verification_data)})
        except:
            test_data.append({"Network": "Empty",
                    "Query": {"Failover": (int)(Q_fails), "Instance": Q_instance, "Type": Q_type},
                    "Verification": tuple(sorted_verification_data)})


    except json.decoder.JSONDecodeError as e:
        failed_test_cases = failed_test_cases + 1
        print(f)

def print_reduction_win(lst):
    i = -1
    for E in lst:
        i = i + 1
        if i == 0:
            reduction = "R0"
        elif i == 1:
            reduction = "R1"
        elif i == 2:
            reduction = "R2"
        elif i == 3:
            reduction = "R3"
        elif i == 4:
            reduction = "R4"
        elif i == 5:
            reduction = "Even"
        elif i == 6:
            reduction = "Unknown"
        print(reduction + "&" + str(E[1]) + "&" + str(E[2]) + "&" + str(E[3]) + "\\\ \hline")

print("Reduction test")
print_reduction_win(post_r)

print("Number of finished test cases: " + str(finished_test_cases))
print("Number of failed test cases: " + str(failed_test_cases))
print("Number of total test cases: " + str(finished_test_cases + failed_test_cases))