import os

"""
Python script for running your code on many inputs and comparing with desired outputs.
The inputs are very large (generated randomly) and the outputs are even larger.
Your executable should be the compilation result of main1.cpp with your code.
This script will run your code with valgrind, and also compare your outputs with the desired outputs.
If nothing is wrong, "Ok" will be printed.
If there are errors, there will be a message, and you can check the details in the logs, or
run the diff specifically on the problematic file.

Usage:
python3 run_tests.py

Make sure you're running python 3.6 and upwards.
Make sure you're running the script from the correct directory (that fits the relative paths that you give for
the executable and the test files)
Configure the correct paths for BASE_INPUT_DIR, BASE_DESIRED_OUTPUT_DIR .... if needed.
Make sure to create the directories BASE_INPUT_DIR, BASE_DESIRED_OUTPUT_DIR, BASE_MY_OUTPUT_DIR before running 
the script. BASE_INPUT_DIR should hold the input files, BASE_DESIRED_OUTPUT_DIR should hold the output files.
BASE_MY_OUTPUT_DIR should be empty before first run.
"""

RUN_MY = True # Set to false if your code has not changed and you don't want to waste time
RUN_DIFF = True

BASE_INPUT_DIR = "./tests"
BASE_DESIRED_OUTPUT_DIR = "./ref_results"
BASE_MY_OUTPUT_DIR = "./cpp_out"
MY_EXEC = "./a.out"

if __name__ == '__main__':
    os.system("> valgrind_log.txt")
    os.system("> diff_log.txt")
    for filename in os.listdir(BASE_INPUT_DIR):
        print('Running %s...' % filename)
        valgrind_exit_code = diff_exit_code = 0
        if RUN_MY:
            os.system("echo '%s\n' >> valgrind_log.txt" % filename)
            valgrind_exit_code = os.system(
                'valgrind --leak-check=full --show-leak-kinds=all --log-fd=9 9>>valgrind_log.txt --error-exitcode=1 %s < %s > %s' %(
                MY_EXEC, os.path.join(BASE_INPUT_DIR, filename), os.path.join(BASE_MY_OUTPUT_DIR, 'cpp_out' + filename)
            ))
            if valgrind_exit_code == 1:
                print('Valgrind errors for %s. Check out valgrind_log.txt' % filename)
        if RUN_DIFF:
            os.system("echo '%s\n' >> diff_log.txt" % filename)
            diff_exit_code = os.system('diff %s %s >> diff_log.txt' % (
                os.path.join(BASE_DESIRED_OUTPUT_DIR, filename[:-3] + "out"), os.path.join(BASE_MY_OUTPUT_DIR, 'cpp_out' + filename)
            ))
            if diff_exit_code != 0:
                print('Difference in outputs for %s. Check out diff_log.txt' % filename)
        if valgrind_exit_code == 0 and diff_exit_code == 0:
            print('Ok')