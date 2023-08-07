***Lab3: MMU***

This project implements a virtual memory management unit which maps the virtual address spaces of multiple processes onto physical frames using page table translation. 
The assumption is multiple processes, each with its own virtual address space of 64 pages. Number of physical pages is 128 or less. Input/output formatting can be found in spec pdf.

The simulation is continuous-time. It supports FIFO, Random, Clock, Enhanced Second Chance, Aging, and Working Set algorithms.
Compile with makefile. Use gen.py to generate inputs.
