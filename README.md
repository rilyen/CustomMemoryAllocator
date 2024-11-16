# Custom Memory Allocator

A multi-threaded memory allocator that supports first-fit and best-fit allocation algorithms. Memory usage is tracked with linked-lists for memory fragmentation management. Also implements an in-place compaction feature to optimize memory usage and minimize fragmentation

# Running the program
A main program is included to show the functionality of the memory allocator.
A makefile is included, so simply type
``make``
in the terminal, and run with
``./myalloc``
