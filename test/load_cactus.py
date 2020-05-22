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
bin_hash_split = bin_hash.split("-")

no_weight_lst = []
wzero_lst = []
hops_lst = []
tunnels_lst = []
latency_lst = []
distance_lst = []
complex_lst = []
failures_lst = []

def manipulate_results(lst):
    return_lst = []
    for l in lst:
        if(l['Data']['result'] == False):
            l['Data']['result'] = True
        elif(l['Data']['result'] == True):
            l['Data']['result'] = False
        if(l['Data']['verification-time'] > 1):
            return_lst.append(l)
    return return_lst

tests =[]
for t in ['Aarnet', 'Agis', 'Bandcon', 'Bellcanada', 'Claranet', 'DeutscheTelekom']:
    for N in range(1,6):
        for k in range(0,4):
            for Q in range(1, 6):
                for QI in range(0, 10):
                    tests.append(t + "-" + str(N) + "-k" + str(k) + "-" + str(Q) + "-" + str(QI))
for f in tests:
    files = []
    verification_compilation = []
    mi_post_veri = []
    for i in range(1,11):
        myfile = Path("results/" + bin_hash + "/E2-R4/" + f + "-" + str(i))
        if myfile.is_file():
            files.append(myfile)

        myfile = Path("results/" + bin_hash + "/E2-R4-Wzero/" + f + "-" + str(i))
        if myfile.is_file():
            files.append(myfile)

        myfile = Path("results/" + bin_hash + "/E2-R4-Wcomplex/" + f + "-" + str(i))
        if myfile.is_file():
            files.append(myfile)

        myfile = Path("results/" + bin_hash + "/E2-R4-Wdistance/" + f + "-" + str(i))
        if myfile.is_file():
            files.append(myfile)

        myfile = Path("results/" + bin_hash + "/E2-R4-Whops/" + f + "-" + str(i))
        if myfile.is_file():
            files.append(myfile)

        myfile = Path("results/" + bin_hash + "/E2-R4-Wtunnels/" + f + "-" + str(i))
        if myfile.is_file():
            files.append(myfile)
        myfile = Path("results/" + bin_hash + "/E2-R4-Wfailures/" + f + "-" + str(i))
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
                continue
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

        queries = manipulate_results(queries)
        # Get universal network stats
        verification_times = []

        for q in queries:
            engine = q['Engine']
            reduction = q['Reduction']
            weight = q['Weight']
            rep = q['Repetation']
            q = q['Data']
            t = next((s for s in verification_times if s['Weight'] == weight), False)
            if t:
                red_list = t['Reduction-time']
                red_list.append(q['reduction-time'])
                t['Reduction-time'] = red_list
                veri_list = t['Verification-time']
                veri_list.append(q['verification-time'])
                t['verification-time'] = veri_list
                com_list = t['Compilation']
                com_list.append(q['compilation-time'])
                t['compilation-time'] = com_list
                total_list = t['Total']
                total_list.append(q['verification-time'] + q['compilation-time'] + q['reduction-time'])
                t['Total'] = total_list
                if len(total_list) == 10:
                    element = { "Total": statistics.median(total_list),
                                "Compilation-time": statistics.median(com_list), "Verification-time": statistics.median(veri_list), 
                                "Reduction-time": statistics.median(red_list), "Output": q['result'], "Mode": q['mode'] }
                    if not weight:
                        no_weight_lst.append(element)
                    elif weight == "Wzero":
                        wzero_lst.append(element)
                    elif weight == "Wcomplex":
                        complex_lst.append(element)
                    elif weight == "Whops":
                        hops_lst.append(element)
                    elif weight == "Wfailures":
                        failures_lst.append(element)
                    elif weight == "Wdistance":
                        distance_lst.append(element)
                    elif weight == "Wtunnels":
                        tunnels_lst.append(element)
                else:
                    verification_times.remove(t)
                    verification_times.append(
                        {"Weight": t['Weight'], "Engine": t['Engine'], "Reduction": t['Reduction'], "Reduction-time": red_list,
                         "Verification-time": veri_list, "Compilation": com_list, "Total": total_list})
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
    except json.decoder.JSONDecodeError as e:
        failed_test_cases = failed_test_cases + 1
        print(f)


def get_cactus_json(name, lst):
    i = 0
    cactus_json_lst = {}
    for l in lst:
        cactus_json_lst["instance" + str(i)] = {"status": l['Output'], "rtime": l['Verification-time']}
        i = i + 1
    cactus_json = {"stats": cactus_json_lst, "preamble": {"benchmark": "weight-set", "prog_alias": name }}
    return cactus_json

i = 0

file = open('mkplot/solver' + str(i) + '.json', 'w')
json.dump(get_cactus_json("no", no_weight_lst), file)
file.close

i = i + 1

file = open('mkplot/solver' + str(i) + '.json', 'w')
json.dump(get_cactus_json("zero", wzero_lst), file)
file.close

i = i + 1

file = open('mkplot/solver' + str(i) + '.json', 'w')
json.dump(get_cactus_json("complex", complex_lst), file)
file.close

i = i + 1

file = open('mkplot/solver' + str(i) + '.json', 'w')
json.dump(get_cactus_json("failures", failures_lst), file)
file.close

i = i + 1

file = open('mkplot/solver' + str(i) + '.json', 'w')
json.dump(get_cactus_json("hops", hops_lst), file)
file.close

i = i + 1

file = open('mkplot/solver' + str(i) + '.json', 'w')
json.dump(get_cactus_json("distance", distance_lst), file)
file.close

i = i + 1

file = open('mkplot/solver' + str(i) + '.json', 'w')
json.dump(get_cactus_json("tunnels", tunnels_lst), file)
file.close


print("no: " + str(len(no_weight_lst)))
print("zero: " + str(len(wzero_lst)))
print("complex: " + str(len(complex_lst)))
print("failures: " + str(len(failures_lst)))
print("hops: " + str(len(hops_lst)))
print("distance: " + str(len(distance_lst)))
print("tunnels: " + str(len(tunnels_lst)))