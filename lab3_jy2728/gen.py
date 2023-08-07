import random
import string
import sys

# usage: python3 gen.py >> {input file}
# randomly generates 1-10 processes
# each with 1-8 VMAs, randomly assigned file map and write protection
# and 30-200 instructions per process
# scheduled in random order
# randomly selects processes to exit

MAX_PROC = 10
MAX_VMA = 8
MAX_VPAGE = 63
vpage_range = range(MAX_VPAGE)
tf = [0, 1]

def generate_processes(file):
    num_proc = random.randint(1, MAX_PROC)
    file.write(str(num_proc))
    file.write("\n#######\n")

    for i in range(num_proc):
        num_vma = random.randint(1, MAX_VMA)
        file.write(str(num_vma))
        file.write('\n')
        vpages = random.sample(vpage_range, k = 2 * num_vma)
        vpages.sort()

        j = 0
        while (j < 2 * num_vma):
            file.write(str(vpages[j]))
            file.write(' ')
            file.write(str(vpages[j+1]))
            file.write(' ')
            wp = random.choice(tf)
            fm = random.choice(tf)
            file.write(str(wp))
            file.write(' ')
            file.write(str(fm))
            file.write('\n')

            j += 2
        file.write("#######\n")

    
    processes = [*range(num_proc)]
    random.shuffle(processes)   # random order of context switches
    INST_COUNT = 0
    for i in processes:
        file.write('c ')
        file.write(str(i))
        file.write('\n')
        num_instr = random.randint(100, 1000)
        INST_COUNT += num_instr

        for j in range(num_instr):  # r/w insructions 
            page = random.choice(vpage_range)
            w = random.choice(tf)
            if w:
                file.write('w ')
            else:
                file.write('r ')
            file.write(str(page))
            file.write('\n')
        e = random.choice(tf)
        if e:   # chance of exit
            file.write('e ')
            file.write(str(i))
            file.write('\n')
        file.write("#######\n")
    file.write("# total instructions: ")
    file.write(str(INST_COUNT))
    file.write('\n')
        


    

if __name__ == '__main__':
    fout = sys.stdout
    generate_processes(fout)  

