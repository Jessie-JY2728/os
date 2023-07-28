# Lab1: Linker

This is a two-pass linker that takes individually compiled onject models and creates a single execuable, by removing external symbol references and resolving module relative addressing.
The input is a sequence of tokens, not necessarily well-formatted. Output contains a symbol table and a memory map. Pass 1 parses the input and verifies the correct syntax and determines the base address for each module and the absolute address for each defined symbol, storing the latter in a symbol table. Pass Two again parses the input and uses the base addresses and the symbol table entries created in pass one to generate the actual output. 

See spec for more details. 

Compile with *make* and the created executable will be named linker1.
Clean up with *make clean*.
