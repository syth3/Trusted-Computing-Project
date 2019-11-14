#! /usr/bin/python3

"""
Probably overkill, but here's a useful script for part 3.1 of the Spectre SEED lab.
To run it, have a directory with only the compiled CacheTime.c file in it (we'll call it the working dir)
Compile CacheTime.c with: gcc -march=native CacheTime.c -o <any name you want>

Run this script with the following syntax:
    ./3.1-helper-script.py <path/to/working/dir (can be a relative path)> <number of times to run compiled CacheTime.c> <name of compiled CacheTime.c>
Example:
    ./3.1-helper-script.py cache-times/ 10000 CacheTime
    Here is what ../cache-times/ looks like in the above example:
        cache-times/
        └── CacheTime
"""

import sys
import os


def create_cpu_cycle_files(path, run_number, executable):
    if not os.path.exists(path):
        os.makedirs(path)
    else:
        file_list = os.listdir(path)
        for file in file_list:
            os.remove(os.path.join(path, file))
    for run in range(1, run_number+1):
        os.system("cd " + path + "; ../" + executable + " > " + str(run) + ".txt")


def analyze_files(file_name_list, path):
    cpu_cycles_by_array = [[], [], [], [], [], [], [], [], [], []]

    for curr_file in file_name_list:
        curr_file_path = os.path.join(path, curr_file)
        open_file = open(curr_file_path, "r")
        array_counter = 0
        for line in open_file:
            cpu_cycles_by_array[array_counter].append(int(parse_string(line)))
            array_counter += 1
        open_file.close()
    
    return cpu_cycles_by_array


def parse_string(string):
    split_colon = string.split(":")[1].strip()
    parse_number = split_colon.split(" ")[0].strip()
    return parse_number


def average_lists(multidemontional_list):
    averaged_list = []
    for curr_list in multidemontional_list:
        averaged_list.append(sum(curr_list) / len(curr_list))
    
    return averaged_list


def main():
    path = sys.argv[1]
    path = os.path.join(path, "run-time-files")
    run_number = int(sys.argv[2])
    executable = sys.argv[3]

    create_cpu_cycle_files(path, run_number, executable)

    file_name_list = os.listdir(path)
    file_name_list = sorted(file_name_list, key=lambda x: int(os.path.splitext(x)[0]))

    cpu_cycle_times = analyze_files(file_name_list, path)

    cpu_cycle_average_times = average_lists(cpu_cycle_times)
    averaged_list = cpu_cycle_average_times

    for i in range(len(averaged_list)):
        print("array" + str(i) + " average cpu cycles:", averaged_list[i])

    print()
    cache_hit_array_average = (averaged_list[3] + averaged_list[7]) / 2
    print ("cache hit array average cpu cycles:", round(cache_hit_array_average, 2))

    cache_miss_array_average = (averaged_list[0] + averaged_list[1] + averaged_list[2] + averaged_list[4] + averaged_list[5] + averaged_list[6] + averaged_list[8] + averaged_list[9]) / 8
    print ("cache miss array average cpu cycles:", round(cache_miss_array_average, 2))


main()
