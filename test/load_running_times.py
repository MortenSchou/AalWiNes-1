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
mpost_post = [[0 for s in range(5)] for i in range(4)]
#Moped vs pre
mpre_pre = [[0 for s in range(5)] for i in range(4)]
mpost_mpre = [[0 for s in range(5)] for i in range(4)]
post_pre = [[0 for s in range(5)] for i in range(4)]

mipost_post = [[0 for s in range(4)] for i in range(4)]
mipre_pre = [[0 for s in range(4)] for i in range(4)]

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
finished_test_cases = 0
failed_test_cases = 0
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
            """
            myfile = Path("results/" + bin_hash + "/E1-R0/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)

            myfile = Path("results/" + bin_hash + "/E1-R1/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)
            
            myfile = Path("results/" + bin_hash + "/E1-R2/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)
            
            myfile = Path("results/" + bin_hash + "/E1-R3/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)
            """
            myfile = Path("results/" + bin_hash + "/E1-R4/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)
            """
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
            """
            myfile = Path("results/" + bin_hash + "/E2-R4/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)
            """
            myfile = Path("results/" + bin_hash + "/E2-R4-Wzero/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)

            myfile = Path("results/" + bin_hash + "/E2-R4-Wcomplex/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)

            myfile = Path("results/" + bin_hash + "/E3-R0/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)

            myfile = Path("results/" + bin_hash + "/E3-R1/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)

            myfile = Path("results/" + bin_hash + "/E3-R2/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)

            myfile = Path("results/" + bin_hash + "/E3-R3/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)
            """
            myfile = Path("results/" + bin_hash + "/E3-R4/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)
            """
            myfile = Path("results/" + bin_hash + "/E4-R0/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)

            myfile = Path("results/" + bin_hash + "/E4-R1/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)

            myfile = Path("results/" + bin_hash + "/E4-R2/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)

            myfile = Path("results/" + bin_hash + "/E4-R3/" + f + "-" + str(i))
            if myfile.is_file():
                files.append(myfile)
            """            
            myfile = Path("results/" + bin_hash + "/E4-R4/" + f + "-" + str(i))
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
                if(len(test_type) == 3):
                    weight = test_type[2]
                    queries.append({"Name": name, "Repetation": test_name[5], "Reduction": reduction, "Engine": engine, "Weight": weight, "Data": jd["answers"]["Q1"]})
                else:
                    queries.append({"Name": name, "Repetation": test_name[5], "Reduction": reduction, "Engine": engine, "Weight": False, "Data": jd["answers"]["Q1"]})

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
                weight = q['Weight']
                rep = q['Repetation']
                q = q['Data']
                t = next((s for s in verification_times if s['Engine'] == engine and s['Reduction'] == reduction), False)
                if t:
                    red_list = t['Reduction-time']
                    red_list.append(q['reduction-time'])
                    t['Reduction-time'] = red_list
                    veri_list = t['Verification-time']
                    veri_list.append(q['verification-time'])
                    t['Verification-time'] = veri_list
                    com_list = t['Compilation']
                    com_list.append(q['compilation-time'])
                    t['Compilation'] = com_list
                    total_list = t['Total']
                    total_list.append(q['verification-time'] + q['compilation-time'] + q['reduction-time'])
                    t['Total'] = total_list
                    if len(total_list) == 10:
                        #verification_compilation.append({"MI": mi, "Engine": engine, "Reduction": reduction, "Total": Average(total_time),
                    #        "Compilation-time": Average(compilation_time), "Verification-time": Average(verification_time), "Reduction-time": Average(reduction_time), "Output": q['result']})
                        verification_compilation.append({"Weight": weight, "Engine": engine, "Reduction": reduction, "Total": statistics.median(total_list),
                            "Compilation-time": statistics.median(com_list), "Verification-time": statistics.median(veri_list), "Reduction-time": statistics.median(red_list), "Output": q['result'],
                            "PDA_states_reduction": q["pda_states_rules"][0], "PDA_rules_reduction": q["pda_states_rules"][1]})
                    else:
                        verification_times.append(
                            {"Weight": t['Weight'], "Engine": t['Engine'], "Reduction": t['Reduction'],
                             "Reduction-time": red_list,
                             "Verification-time": veri_list, "Compilation": com_list, "Total": total_list})
                        verification_times.remove(t)
                else:
                    red_list = []
                    veri_list = []
                    com_list = []
                    total_list = []
                    red_list.append(q['reduction-time'])
                    veri_list.append(q['verification-time'])
                    com_list.append(q["compilation-time"])
                    total_list.append(q['verification-time'] + q['compilation-time'] + q['reduction-time'])
                    verification_times.append({"Weight": weight, "Engine": engine, "Reduction": reduction, "Reduction-time": red_list, "Verification-time": veri_list, "Compilation": com_list, "Total": total_list})



            sorted_verification_data = sorted(
                verification_compilation, key=lambda k: k['Verification-time'])

            Percentage = 30/100
            Second = 0.001
            # Find fastest engine for verification + reduction
            #for r in ["R0", "R1", "R2", "R3", "R4"]:
            for r in ["R4"]:
                find_win("E1", "E2", r, mpost_post, Percentage, Second, sorted_verification_data)
                find_win("E4", "E3", r, mpre_pre, Percentage, Second, sorted_verification_data)
                find_win("E1", "E4", r, mpost_mpre, Percentage, Second, sorted_verification_data)
                find_win("E2", "E3", r, post_pre, Percentage, Second, sorted_verification_data)

            Nodes = ""
            labels = ""
            Rules = ""
            States = ""
            PDARules = ""
            States_r = ""
            Rules_r = ""

            for j in range(len(queries)):
                try:
                    Nodes = queries[j]['Data']["network_node_size"]
                    labels = queries[j]['Data']["network_label_size"]
                    Rules = queries[j]['Data']["network_rules_size"]
                    States = queries[j]['Data']["pda_states_rules"][0]
                    PDARules = queries[j]['Data']["pda_states_rules"][1]
                    States_r = queries[j]['Data']["pda_states_rules_reduction"][0]
                    Rules_r = queries[j]['Data']["pda_states_rules_reduction"][1]
                except:
                    continue
            
            test_data.append({"Network": {"Nodes": Nodes, "Labels": labels, "Rules": Rules,
                                    "States": States, "PDA_Rules": PDARules,
                                    "States_reduction": States_r, "Rules_reduction": Rules_r},
                        "Query": {"Failover": (int)(Q_fails), "Instance": Q_instance, "Type": Q_type},
                        "Verification": tuple(sorted_verification_data)})

        except json.decoder.JSONDecodeError as e:
            failed_test_cases = failed_test_cases + 1
            print(f)


def printwin(lst):
    i = -1
    for E in lst:
        i = i + 1
        if i == 0:
            engine = "EngineA"
        elif i == 1:
            engine = "EngineB"
        elif i == 2:
            engine = "Even"
        elif i == 3:
            engine = "Unknown"
        print(engine + "&" + str(E[4]) + "\\\ \hline")


#Latex print fastest engine on fastest reducktion = 0
def printline(lst, bi, ei):
    for t in lst[bi:ei]:
        print(str(t['Test']['Network']['Nodes']) + "&" +  str(t['Test']['Network']['Rules']) + "&" + str(t['Test']['Network']['Labels']) 
            + "&" + str(t['Test']['Network']['States_reduction']) + "&" + str(t['Test']['Network']['Rules_reduction']) + "& F:" + 
            str(t['Test']['Query']['Failover']) + " I:" + str(t['Test']['Query']['Instance']) + " T:" + str(t['Test']['Query']['Type']) +
            "&" + str(t['Output']) + "&" + str(round(t['FT'], 4)) + "&" + str(round(t['NT'], 4)) + "&" + str(round(t['Ratio'], 4)) + "\\\ \hline")
    print("$\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$\\\ \hline")

def printline_r(lst, bi, ei):
    for t in lst[bi:ei]:
        print(str(t['Test']['Network']['Nodes']) + "&" +  str(t['Test']['Network']['Rules']) + "&" + str(t['Test']['Network']['Labels']) 
            + "&" + str(t['FSR']) + "&" + str(t['FRR']) + "&" + str(t['NRR']) + "& F:" + 
            str(t['Test']['Query']['Failover']) + " I:" + str(t['Test']['Query']['Instance']) + " T:" + str(t['Test']['Query']['Type']) +
            "&" + str(t['Output']) + "&" + str(round(t['FT'], 4)) + "&" + str(round(t['NT'], 4)) + "&" + str(round(t['Ratio'], 4)) + "\\\ \hline")
    print("$\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$ & $\\vdots$\\\ \hline")

def printlines(size, list, list_false = []):
    print("\\begin{table}[H]\n\\centering\n\\begin{adjustwidth}{-2cm}{}\n\\begin{tabular}{|c|c|c|c|c|c|c|c|c|}\n \hline \n \\rowcolor[HTML]{9B9B9B}\n \\textbf{Nodes}& \\textbf{Rules} & \\textbf{Labels} & \\textbf{PDA-S} & \\textbf{PDA-R} & \\textbf{Query} & \\textbf{Output} & \\textbf{V1} & \\textbf{V2} & \\textbf{Ratio}\\\ \hline")

    sorted_engine_ratio = sorted(list, key=lambda k: k['Ratio'])
    printline(sorted_engine_ratio, 0, size)
    mid = int(len(sorted_engine_ratio)/2)
    printline(sorted_engine_ratio, mid, mid + size)
    last = int(len(sorted_engine_ratio))
    printline(sorted_engine_ratio, last - size, last)

    if list_false:
        print("False")
        print("\\begin{table}[H]\n\\centering\n\\begin{adjustwidth}{-2cm}{}\n\\begin{tabular}{|c|c|c|c|c|c|c|c|c|}\n \hline \n \\rowcolor[HTML]{9B9B9B}\n \\textbf{Nodes}& \\textbf{Rules} & \\textbf{Labels} & \\textbf{PDA-S} & \\textbf{PDA-R} & \\textbf{Query} & \\textbf{Result} & \\textbf{V1} & \\textbf{V2} & \\textbf{Ratio}\\\ \hline")
        sorted_engine_ratio_false = sorted(list_false, key=lambda k: k['Ratio'])
        printline(sorted_engine_ratio_false, 0, size)
        mid = int(len(sorted_engine_ratio_false)/2)
        printline(sorted_engine_ratio_false, mid, mid + size)
        last = int(len(sorted_engine_ratio_false))
        printline(sorted_engine_ratio_false, last - size, last)

def printlines_r(list):
    print("\\begin{table}[H]\n\\centering\n\\begin{adjustwidth}{-2cm}{}\n\\begin{tabular}{|c|c|c|c|c|c|c|c|c|c|c|c|}\n \hline \n \\rowcolor[HTML]{9B9B9B}\n \\textbf{Nodes}& \\textbf{Rules} & \\textbf{Labels} & \\textbf{PDA-S} & \\textbf{PDA-R1} & \\textbf{PDA-R2} & \\textbf{Query} & \\textbf{Result} & \\textbf{V1} & \\textbf{V2} & \\textbf{Ratio}\\\ \hline")
    sorted_engine_ratio = sorted(list, key=lambda k: k['Ratio'])
    printline_r(sorted_engine_ratio, 0, 5)
    mid = int(len(sorted_engine_ratio)/2)
    printline_r(sorted_engine_ratio, mid, mid + 5)
    last = int(len(sorted_engine_ratio))
    printline_r(sorted_engine_ratio, last - 5, last)


def get_ratio(engineA, engineB):
    engine_ratio = []
    engine_ratio_false = []
    engine_ratio_all = []

    for t in test_data:
        for r in ["R0", "R1", "R2", "R3", "R4"]:
        #for r in ["R4"]:
            try:
                first = next(s for s in t['Verification'] if s['Engine'] == engineA and s['Reduction'] == r)
                fastest_time = first['Verification-time']
                next_veri = next(s for s in t['Verification'] if s['Engine'] == engineB and s['Reduction'] == r)
                #Remove very fast cases on both engines
                if  next_veri['Verification-time'] < 0.5 and fastest_time < 0.5: continue
                if  abs(next_veri['Verification-time'] - fastest_time) < 0.001: continue
                if  first['Mode'] != next_veri['Mode'] : continue
                if  first['Output'] != next_veri['Output'] : continue
            except:
                continue
            try:
                ratio =  fastest_time / next_veri['Verification-time']
                if ratio < 1:
                    ratio = -1 / ratio
                #Get and print Output result
                engine_ratio_all.append({"Test": t, "FT": fastest_time, "Output": first['Output'], "NT": next_veri['Verification-time'], "NE": next_veri['Engine'], "Ratio": ratio})
                if first['Output'] == True:
                    engine_ratio.append({"Test": t, "FT": fastest_time, "Output": first['Output'], "NT": next_veri['Verification-time'], "NE": next_veri['Engine'], "Ratio": ratio})
                else:
                    engine_ratio_false.append({"Test": t, "FT": fastest_time, "Output": first['Output'], "NT": next_veri['Verification-time'], "NE": next_veri['Engine'], "Ratio": ratio})
            except:
                print("some")
                    
    #print(engineA + engineB)
    #printlines(engine_ratio, engine_ratio_false)

    print("All")
    print(engineA + engineB)
    printlines(5, engine_ratio_all)

def get_ratio_MI(engineA, engineB, parameter):
    engine_ratio_all = []
    for t in test_data:
        #for r in ["R0", "R1", "R2", "R3", "R4"]:
        try:
            first = next(s for s in t['Verification'] if s['Engine'] == engineA and s['Reduction'] == "R0")
            fastest_time = first[parameter]
            next_veri = next(s for s in t['Verification'] if s['Engine'] == engineB and s['Reduction'] == "R4")
            next_time = next_veri[parameter]
            #Remove very fast cases on both engines
            if  next_time < 0.5 and fastest_time < 0.5: continue
            if  abs(next_time - fastest_time) < 0.001: continue
        except:
            continue
        try:
            ratio =  fastest_time / next_time
            if ratio < 1:
                ratio = -1 / ratio
            #Get and print Output result
            engine_ratio_all.append({"Test": t, "FSR": first['PDA_states_reduction'], "FRR": first['PDA_rules_reduction'], "NSR": next_veri['PDA_states_reduction'], "NRR": next_veri['PDA_rules_reduction'], 
                                    "FT": fastest_time, "Output": first['Output'], "NT": next_time, "NE": next_veri['Engine'], "Ratio": ratio})
        except:
            print("some error")

    print("All")
    print(engineA + engineB)
    printlines_r(engine_ratio_all)

def get_ratio_weight(engine, parameter, weight_t1, weight_t2 = ""):
    engine_ratio_all = []
    engine_ratio = []
    engine_ratio_false = []

    for t in test_data:
        try:
            first = next(s for s in t['Verification'] if s['Engine'] == engine and s['Weight'] == weight_t1)
            fastest_time = first[parameter]
            if weight_t2 == "":
                next_veri = next(s for s in t['Verification'] if s['Engine'] == engine and s['Weight'] == False)
                next_time = next_veri[parameter]
            else:
                next_veri = next(s for s in t['Verification'] if s['Engine'] == engine and s['Weight'] == weight_t2)
                next_time = next_veri[parameter]
            #Remove very fast cases on both engines
            if  next_time < 0.5 and fastest_time < 0.5: continue
            if  abs(next_time - fastest_time) < 0.001: continue
            if  first['Mode'] != next_veri['Mode'] : continue
        except:
            continue
        try:
            ratio =  fastest_time / next_time
            if ratio < 1:
                ratio = -1 / ratio
            #Get and print Output result
            engine_ratio_all.append({"Test": t, "FT": fastest_time, "Output": first['Output'], "NT": next_time, "NE": next_veri['Engine'], "Ratio": ratio})
            if first['Output'] == True:
                engine_ratio.append({"Test": t, "FT": fastest_time, "Output": first['Output'], "NT": next_veri['Verification-time'], "NE": next_veri['Engine'], "Ratio": ratio})
            elif first['Output'] == False:
                engine_ratio_false.append({"Test": t, "FT": fastest_time, "Output": first['Output'], "NT": next_veri['Verification-time'], "NE": next_veri['Engine'], "Ratio": ratio})
        except:
            print("some error")
    print("False")
    printlines(5, engine_ratio_false)
    print("All")
    printlines(5, engine_ratio_all)

#get_ratio_MI("E2", "E2", "Verification-time")
#get_ratio_MI("E3", "E3", "Verification-time")

#print("M_post vs Post")
#get_ratio("E1","E2")
#print("M_pre vs Pre")
#get_ratio("E4","E3")
#print("Post vs Pre")
#get_ratio("E2","E3")

#Print fastest overall time
print("POSTvsPre")
printwin(post_pre)
print("mPOSTvsmPre")
printwin(mpost_mpre)
print("MopedPostvsPOST")
printwin(mpost_post)
print("MopedPrevsPre")
printwin(mpre_pre)

#print("Ratio E2, E2")
#get_ratio_MI("E2", "E2", "Total")
"""
print("Ratio E2W, E2")
get_ratio_weight("E2", "Verification-time", "Wzero")

print("Ratio E2W, E2Wcomplex")
get_ratio_weight("E2", "Verification-time", "Wzero", "Wcomplex")
"""
print("Number of finished test cases: " + str(finished_test_cases))
print("Number of failed test cases: " + str(failed_test_cases))
print("Number of total test cases: " + str(finished_test_cases + failed_test_cases))