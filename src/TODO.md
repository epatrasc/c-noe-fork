# TODO

## main.c

- create a list of process alive
- birth_death: kill a random process
    - get pid
    - send signal kill
    - waitpid : sure?
    - set alive = 0  on shared memory
- free memory societa

## child_a / child_b

- handle kill signal
    + unlock semaphores
    + free memory
    + delete fifo
