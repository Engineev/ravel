import os
import sys
import subprocess

compilers = {
    'gcc': 'riscv32-unknown-linux-gnu-gcc -S -fno-section-anchors -O%d ' +
           '-o test.s test.c',
    # 'clang': 'clang-9 -S -emit-llvm -O%d -o test.ll test.c; ' +
    #          'llc-9 --march=riscv32 -mattr=+m -o test.s test.ll'
    # It seems that this command sequence is wrong...
}

test_cases = [
    'optim/sha_1',
    'optim/pi',
    'optim/humble',
    'optim/segtree',
    'optim/lunatic',
    'optim/maxflow',
    'optim/dijkstra',
    'optim/lca',
    'optim/binary_tree',
    'optim/kruskal'
]
test_cases.sort()

ravel_cmd = './ravel --oj-mode --enable-cache --timeout=30000000000 ' + \
            '>ravel.out 2>/dev/null'
diff_cmd = 'diff test.out test.ans -q >/dev/null 2>/dev/null'

color_red = "\033[0;31m"
color_green = "\033[0;32m"
color_none = "\033[0m"


def execute(cmd):
    # print("running: %s" % cmd)
    return subprocess.run(cmd, shell=True, executable="/bin/bash")


# build
directory = os.path.dirname(os.path.abspath(__file__))
os.chdir(os.path.join(directory, '..'))
os.system('mkdir build; cd build')
os.chdir(os.path.join(os.getcwd(), 'build'))
if os.system('cmake .. && make'):
    print("Build failed")
    exit(1)
os.chdir(directory)
os.system('cp ../build/src/ravel ./')

# test
print("%d test cases." % len(test_cases))
failed_test_cases = []
for test_case in test_cases:
    print(test_case + ': ', end='\t', flush=True)
    execute('cp %s.c ./test.c' % test_case)
    execute('cp %s.in ./test.in' % test_case)
    execute('cp %s.ans ./test.ans' % test_case)
    for compiler, compiler_cmd in compilers.items():
        for opt_level in [0, 1, 2]:
            execute(compiler_cmd % opt_level)
            identifier = "%s-O%d" % (compiler, opt_level)
            ravel_res = execute(ravel_cmd)
            if ravel_res.returncode:
                print(color_red + identifier + '(RE)' + color_none,
                      end='\t', flush=True)
                failed_test_cases.append(test_case + '(' + identifier + ')')
                continue
            diff_res = execute(diff_cmd)
            if diff_res.returncode:

                print(color_red + identifier + '(WA)' + color_none,
                      end='\t', flush=True)
                failed_test_cases.append(test_case + '(' + identifier + ')')
                continue
            with open('ravel.out', 'r') as f:
                lines = f.readlines()
                for line in lines:
                    if not line.startswith('time: '):
                        continue
                    time = int(line[6:])
                    break
            print(color_green + identifier + '(' + f'{time:,}' + ')'
                  + color_none, end='\t', flush=True)
    print('')

execute('rm ravel test.ans test.c test.in test.out test.s ravel.out')
if len(failed_test_cases) == 0:
    print('Passed all test cases')
    exit(0)
print("Failed: ")
for test_case in failed_test_cases:
    print(test_case)

