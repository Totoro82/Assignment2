#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdatomic.h>

int num_threads;

typedef struct node {
    int node_id; //a unique ID assigned to each node
    struct node *next;
} Node;

_Atomic(Node *) top = NULL; // top of the stack
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // mutex for locking

atomic_int next_node_id = 0;
atomic_int thread_id = 0;

/*Option 1: Mutex Lock*/
void push_mutex() {
    Node *new_node = malloc(sizeof(Node));
    if (new_node == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    new_node->node_id = atomic_fetch_add(&next_node_id, 1); //fetches next id and increments atomically
    // cs
    pthread_mutex_lock(&mutex);
    new_node->next = top;
    top = new_node;
    //end cs
    pthread_mutex_unlock(&mutex);

    //update top of the stack below
    //assign a unique ID to the new node
}

int pop_mutex() {
    Node *old_node;
    int id = -1;
    //update top of the stack below
    //cs
    pthread_mutex_lock(&mutex);

    old_node = top;
    if (old_node != NULL) {
        top = old_node->next;
        id = old_node->node_id;
        free(old_node);
    }
    //end cs
    pthread_mutex_unlock(&mutex);

    return id;
}

/*Option 2: Compare-and-Swap (CAS)*/
void push_cas() {
    Node *old_node;
    Node *new_node = malloc(sizeof(Node));
    if (new_node == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    new_node->node_id = atomic_fetch_add(&next_node_id, 1);
    do {
        old_node = atomic_load(&top);
        new_node->next = old_node;
    } while (!atomic_compare_exchange_weak(&top, &old_node, new_node));

    //update top of the stack below
    //assign a unique ID to the new node
}

int pop_cas() {
    Node *old_node;
    Node *new_node;
    int id = -1;
    do {
        old_node = atomic_load(&top);
        if (old_node == NULL) {
            return -1; //stack empty
        }
        new_node = old_node->next;
    } while (!atomic_compare_exchange_weak(&top, &old_node, new_node));
    id = old_node->node_id;
    free(old_node);

    //update top of the stack below

    return id;
}

/* the thread function */
void *thread_func(void *arg) {
    int opt = (int) (intptr_t) arg; //transform pointer to int
    /* Assign each thread an id so that they are unique in range [0, num_thread -1 ] */
    int my_id = atomic_fetch_add(&thread_id, 1);

    if (opt == 0) {
        push_mutex();
        push_mutex();
        pop_mutex();
        pop_mutex();
        push_mutex();
    } else {
        push_cas();
        push_cas();
        pop_cas();
        pop_cas();
        push_cas();
    }

    printf("Thread %d: exit\n", my_id);
    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    num_threads = atoi(argv[1]);
    int option = atoi(argv[2]); // 0 for mutex, 1 for CAS

    /* Option 1: Mutex */
    pthread_t *workers = malloc(num_threads * sizeof(pthread_t)); // array of threads
    for (int i = 0; i < num_threads; i++) {
        // create threads
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&workers[i], &attr, thread_func, (void*)(intptr_t)option);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(workers[i], NULL); // wait for all threads to finish
    }
    free(workers);
    //Print out all remaining nodes in Stack
    if (option == 0) {
        pthread_mutex_lock(&mutex);
        Node *current = top;
        int count = 0;
        while (current != NULL) {
            count++;
            current = current->next;
        }
        pthread_mutex_unlock(&mutex);
        printf("Mutex: Remaining nodes %d \n", count);

        /*free up resources properly */
        pthread_mutex_lock(&mutex);
        current = top;
        while (current != NULL) {
            Node *temporal = current;
            current = current->next;
            free(temporal);
        }
        top = NULL;
        pthread_mutex_unlock(&mutex);
        pthread_mutex_destroy(&mutex);
    } else {
        Node *current = atomic_load(&top);
        int count = 0;
        while (current != NULL) {
            count++;
            current = current->next;
        }
        printf("CAS: Remaining nodes %d \n", count);

        /*free up resources properly */
        current = atomic_load(&top);
        while (current != NULL) {
            Node *temporal = current;
            current = current->next;
            free(temporal);
        }
        top = NULL;
    }
    return 0;
}
