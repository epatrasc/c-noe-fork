# TODO

## main.c

- create a list of process alive: DONE by filtering alive
- wait update societa when a process is dead: OK
- birth_death: kill a random process
    - get pid: OK
    - send signal kill: OK
    - waitpid: OK
    - set alive = 0  on shared memory 
        - check child type, only A is on shared memory
        - use semaphores
- free memory societa
- handle END SIM
    - ...

## child_a / child_b

- handle kill signal
    + unlock semaphores
    + free memory
    + delete fifo
