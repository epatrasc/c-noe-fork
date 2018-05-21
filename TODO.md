# TODO

## main.c

- read PARAM from file or argv
- use signal SIGUSER1/2 to trigger the read from the queue
- handle stats counters
- handle msg queue read with a loop
- delete msg queue

- create a list of process alive: ![#c5f015](https://placehold.it/15/c5f015/000000?text=+) DONE by filtering alive
- wait update societa when a process is dead: ![#c5f015](https://placehold.it/15/c5f015/000000?text=+) OK
- birth_death: kill a random process
    - get pid: ![#c5f015](https://placehold.it/15/c5f015/000000?text=+) OK
    - send signal kill: ![#c5f015](https://placehold.it/15/c5f015/000000?text=+) OK
    - waitpid
        - handle stats
    - set alive = 0  on shared memory 
        - check child type, only A is on shared memory ![#c5f015](https://placehold.it/15/c5f015/000000?text=+)  OK
        - use semaphores
- free memory societa
- handle END SIM
    - kill every process by SIGTERM: ![#c5f015](https://placehold.it/15/c5f015/000000?text=+) OK
    - delete shm semaphore: ![#c5f015](https://placehold.it/15/c5f015/000000?text=+) OK

## child_a / child_b

- handle kill signal
    + unlock semaphores: ![#c5f015](https://placehold.it/15/c5f015/000000?text=+) OK
    + free memory: ![#c5f015](https://placehold.it/15/c5f015/000000?text=+) OK
    + delete fifo::![#c5f015](https://placehold.it/15/c5f015/000000?text=+)  OK

## child_a
- after x request make the selection less selective: ![#c5f015](https://placehold.it/15/c5f015/000000?text=+) OK