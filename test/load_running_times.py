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
moped_post = [[0 for s in range(4)] for i in range(4)]
#Moped vs pre
moped_pre = [[0 for s in range(4)] for i in range(4)]
post_pre = [[0 for s in range(4)] for i in range(4)]

mipost_post = [[0 for s in range(4)] for i in range(4)]
mipre_pre = [[0 for s in range(4)] for i in range(4)]

bin_hash_split = bin_hash.split("-")

def Average(lst): 
    return sum(lst) / len(lst) 

def min_dif(lst):
    lst2 = lst[10:20]
    lst1 = lst[0:10]
    return min([abs(x - y) for x, y in zip(lst1, lst2)])

def find_win_MI(enginea, engineb, reduction, con_win, Percentage, Second, data):
    list_win = [s for s in data if (s['Reduction'] == reduction and s['Engine'] == enginea)]
    i = int(reduction[1:])
    if len(list_win) == 2:
        #Even
        if (list_win[1]['Verification-time'] - list_win[0]['Verification-time']) / list_win[1]['Verification-time'] < Percentage or list_win[1]['Verification-time'] - list_win[0]['Verification-time'] < Second:
            con_win[2][i] = con_win[2][i] + 1
        elif list_win[0]['MI'] == 1:
            con_win[0][i] = con_win[0][i] + 1
        elif list_win[0]['MI'] == 0:
            con_win[1][i] = con_win[1][i] + 1
    else:
        con_win[3][i] = con_win[3][i] + 1

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
        MI_Files = []
        verification_compilation = []
        mi_post_veri = []
        for i in range(1,11):
            myfile = Path("results/" + bin_hash + "/E2-R0/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)

            myfile = Path(
                "results/" + bin_hash + "/E3-R0/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)

            myfile = Path("results/" + bin_hash_split[0] + "-" + bin_hash_split[1] + "-1" + "/E2-R0/" + f + "-" + str(i))
            if myfile.is_file():
                MI_Files.append(myfile)

            myfile = Path(
                "results/" + bin_hash_split[0] + "-"  + bin_hash_split[1] + "-1" + "/E3-R0/" + f + "-" + str(i))
            if myfile.is_file():
                MI_Files.append(myfile)

        try:
            if not files:
                continue
            queries = []

            for i in range(len(files)):
                jd = json.load(open(files[i]))
                name = files[i].parts[2]
                test_type = [s for s in name.split('-')]
                test_name = [s for s in files[i].name.split('-')]
                engine = test_type[0]
                reduction = test_type[1]
                queries.append({"Name": name, "Repetation": test_name[4], "Reduction": reduction, "Engine": engine, "Data": jd["answers"]["Q1"], "MI": 0})

            for i in range(len(MI_Files)):
                jd_mi = json.load(open(MI_Files[i]))
                name = MI_Files[i].parts[2]
                test_type = [s for s in name.split('-')]
                test_name = [s for s in files[i].name.split('-')]
                engine = test_type[0]
                reduction = test_type[1]
                queries.append({"Name": name, "Repetation": test_name[4], "Reduction": reduction, "Engine": engine, "Data": jd_mi["answers"]["Q1"], "MI": 1})


            # Get universal network stats
            test_numbers = [int(s[1:])
                            for s in f.split('-') if s[1:].isdigit()]
            test_name = [s for s in f.split('-')]
            Q_type = test_numbers[0]
            Q_fails = test_numbers[1]
            verification_times = []


            i = j = 1
            for q in queries:
                engine = q['Engine']
                reduction = q['Reduction']
                mi = q['MI']
                rep = q['Repetation']
                q = q['Data']
                t = next((s for s in verification_times if s['Engine'] == engine and s['MI'] == mi), False)
                if t:
                    red_list = t['Reduction']
                    red_list.append(q['reduction-time'])
                    t['Reduction'] = red_list
                    veri_list = t['Verification-time']
                    veri_list.append(q['verification-time'])
                    t['Verification-time'] = veri_list
                    com_list = t['Compilation']
                    com_list.append(q['compilation-time'])
                    t['Compilation'] = com_list
                    total_list = t['Total']
                    total_list.append(q['verification-time'] + q['compilation-time'] + q['reduction-time'])
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
                    verification_times.append({"MI": mi, "Engine": engine, "Reduction": red_list, "Verification-time": veri_list, "Compilation": com_list, "Total": total_list})



            sorted_verification_data = sorted(
                verification_compilation, key=lambda k: k['Verification-time'])

            Percentage = 20/100
            Second = 0.00001
            # Find fastest engine for verification + reduction
            for r in ["R0", "R1", "R2", "R3"]:
                #find_win("E1", "E2", r, moped_post, Percentage, Second, sorted_verification_data)
                #find_win("E1", "E3", r, moped_pre, Percentage, Second, sorted_verification_data)
                find_win("E2", "E3", r, post_pre, Percentage, Second, sorted_verification_data)



            test_data.append({"Network": {"Nodes": queries[0]['Data']["network_node_size"], "Labels": queries[0]['Data']["network_label_size"], "Rules": queries[0]['Data']["network_rules_size"],
                                    "States": queries[0]['Data']["pda_states_rules"][0], "PDA_Rules": queries[0]['Data']["pda_states_rules"][1],
                                    "States_reduction": queries[0]['Data']["pda_states_rules_reduction"][0], "Rules_reduction": queries[0]['Data']["pda_states_rules_reduction"][1]},
                        "Query": {"Failover": (int)(Q_fails), "Type": Q_type},
                        "Verification": tuple(sorted_verification_data)})

        except json.decoder.JSONDecodeError as e:
            print(f)


def printwin(lst):
    i = -1
    for E in lst:
        i = i + 1
        if i == 0:
            engine = "MI\_post"
        elif i == 1:
            engine = "Post"
        elif i == 2:
            engine = "Even"
        elif i == 3:
            engine = "Unknown"
        print(engine + "&" + str(E[0]) + "&" + str(E[1]) + "&" + str(E[2]) + "&" + str(E[3]) + "\\\ \hline")


#Latex print fastest engine on fastest reducktion = 0
def printline(list, bi, ei):
    for t in list[bi:ei]:
        print(str(t['Test']['Network']['Nodes']) + "&" +  str(t['Test']['Network']['Rules']) + "&" + str(t['Test']['Network']['Labels']) 
            + "&" + str(t['Test']['Network']['States_reduction']) + "&" + str(t['Test']['Network']['Rules_reduction']) + "&" + str(t['Test']['Query']) + "&" + str(t['R'])
            + "&" + str(round(t['FT'], 4)) + "&" + str(round(t['NT'], 4)) + "&" + str(round(t['Ratio'], 4)) + "\\\ \hline")
    print("$\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$\\\ \hline")

def printlines(list, list_false):
    print("True")
    print("\\begin{table}[H]\n\\centering\n\\begin{adjustwidth}{-2cm}{}\n\\begin{tabular}{|c|c|c|c|c|c|c|c|c|c|}\n \hline \n \\rowcolor[HTML]{9B9B9B}\n \\textbf{Nodes}& \\textbf{Rules} & \\textbf{Labels} & \\textbf{PDA-S} & \\textbf{PDA-R} & \\textbf{Query} & \\textbf{R} & \\textbf{V1} & \\textbf{V2} & \\textbf{Ratio}\\\ \hline")

    sorted_engine_ratio = sorted(list, key=lambda k: k['Ratio'])
    printline(sorted_engine_ratio, 0, 5)
    mid = int(len(sorted_engine_ratio)/2)
    printline(sorted_engine_ratio, mid, mid + 5)
    last = int(len(sorted_engine_ratio))
    printline(sorted_engine_ratio, last - 5, last)

    print("False")
    print("\\begin{table}[H]\n\\centering\n\\begin{adjustwidth}{-2cm}{}\n\\begin{tabular}{|c|c|c|c|c|c|c|c|c|c|}\n \hline \n \\rowcolor[HTML]{9B9B9B}\n \\textbf{Nodes}& \\textbf{Rules} & \\textbf{Labels} & \\textbf{PDA-S} & \\textbf{PDA-R} & \\textbf{Query} & \\textbf{R} & \\textbf{V1} & \\textbf{V2} & \\textbf{Ratio}\\\ \hline")
    sorted_engine_ratio_false = sorted(list_false, key=lambda k: k['Ratio'])
    printline(sorted_engine_ratio_false, 0, 5)
    mid = int(len(sorted_engine_ratio_false)/2)
    printline(sorted_engine_ratio_false, mid, mid + 5)
    last = int(len(sorted_engine_ratio_false))
    printline(sorted_engine_ratio_false, last - 5, last)

def get_ratio(engineA, engineB):
    engine_ratio = []
    engine_ratio_false = []

    for t in test_data:
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
                    
    print(engineA + engineB)
    printlines(engine_ratio, engine_ratio_false)

def get_ratio_MI(engineA, engineB):
    engine_ratio = []
    engine_ratio_false = []

    for t in test_data:
        for r in ["R0", "R1", "R2", "R3"]:
            try:
                first = next(s for s in t['Verification'] if s['Engine'] == engineA and s['Reduction'] == r and s['MI'] == 1)
                fastest_time = first['Verification-time']
                next_veri = next(s for s in t['Verification'] if s['Engine'] == engineB and s['Reduction'] == r and s['MI'] == 0)
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
                    

    print("MI")
    print(engineA + engineB)
    printlines(engine_ratio, engine_ratio_false)

get_ratio_MI("E2", "E2")
get_ratio_MI("E3", "E3")
#get_ratio("E1","E2")
#get_ratio("E1","E3")

#Print fastest overall time
print("MI_POSTvsPOST")
print(post_pre)
printwin(post_pre)

#get_ratio("E2","E3")