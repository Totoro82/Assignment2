# ReadMe
### Authors:
David Gonçalves Bermúdez, Mikael Rudenvald and Linnea Rydberg

### How to run:
1. open a terminal on project root
2. to compile type `make`
```sh
make 
```
3. to run type `make run < N > < O >` (with N being the random input number and O being the option, 0 for Mutex and 1 for CAS)

Example for 4 threads and Mutex:
```sh
make run 4 0
```
4. in case you want to remove the executable file, simply run:
```sh
make clean
```