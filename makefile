CC = gcc
CFLAGS = -Wall -O2

THREADS = 4

all: mainprocess

mainprocess: pthread_stack.c
	$(CC) $(CFLAGS) -o mainprocess pthread_stack.c

run:
	@if [ -z "$(word 2, $(MAKECMDGOALS))" ]; then \
  		echo "You need to specify number of threads as an argument"; \
  	elif ! echo "$(word 2, $(MAKECMDGOALS))" | grep -Eq '^[0-9]+$$'; then \
        echo "Argument must be an integer"; \
  	else \
  		./mainprocess $(word 2, $(MAKECMDGOALS)); \
	fi

%:#this avoids the error: "no rule to make target 'N'
	@:

clean:
	rm -f mainprocess