CC = gcc
CFLAGS = -Wall -O2

THREADS = 4

all: mainprocess

mainprocess: pthread_stack.c
	$(CC) $(CFLAGS) -o mainprocess pthread_stack.c

run:
	@if [ -z "$(word 2, $(MAKECMDGOALS))" ]; then \
		echo "Usage: make run <num_threads> <option>"; \
		echo "<num_threads>: number of threads"; \
		echo "<option>: 0 for mutex, 1 for CAS"; \
	elif [ -z "$(word 3, $(MAKECMDGOALS))" ]; then \
		echo "You need to specify option (0 for mutex, 1 for CAS)"; \
	elif ! echo "$(word 2, $(MAKECMDGOALS))" | grep -Eq '^[0-9]+$$'; then \
		echo "First argument (num_threads) must be an integer"; \
	elif ! echo "$(word 3, $(MAKECMDGOALS))" | grep -Eq '^[01]$$'; then \
		echo "Second argument (option) must be 0 or 1"; \
	else \
		./mainprocess $(word 2, $(MAKECMDGOALS)) $(word 3, $(MAKECMDGOALS)); \
	fi

%:#this avoids the error: "no rule to make target 'N'
	@:

clean:
	rm -f mainprocess

