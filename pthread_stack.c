#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdatomic.h>

int num_threads;

typedef struct node {
    int node_id; //a unique ID assigned to each node
    struct node *next; //pointer to the next node in the stack
} Node;

_Atomic(Node *) top = NULL; // top of the stack
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // mutex for locking

// Atomic integers for unique IDs
atomic_int next_node_id = 0;
atomic_int thread_id = 0;

/*Option 1: Mutex Lock*/
void push_mutex() {
    Node *new_node = malloc(sizeof(Node));
    if (new_node == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    new_node->node_id = atomic_fetch_add(&next_node_id, 1); // assign a unique ID to the new node
    // cs
    pthread_mutex_lock(&mutex);

    new_node->next = top; //update next pointer of new top
    top = new_node;//update top of the stack

    //end cs
    pthread_mutex_unlock(&mutex);
}

int pop_mutex() {
    Node *old_node;
    int id = -1;

    //cs
    pthread_mutex_lock(&mutex);

    old_node = top;//get current top
    if (old_node != NULL) { //stack not empty
        top = old_node->next;//update top to next node in stack
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
    Node *new_node = malloc(sizeof(Node)); //create new node
    if (new_node == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    new_node->node_id = atomic_fetch_add(&next_node_id, 1);// assign a unique ID to the new node
    do {
        old_node = atomic_load(&top); //get current top
        new_node->next = old_node; //update next pointer of new top
    } while (!atomic_compare_exchange_weak(&top, &old_node, new_node)); // assign new node as top if top is still old_node
}

int pop_cas() {
    Node *old_node;
    Node *new_node;
    int id = -1;
    do {
        old_node = atomic_load(&top); //get current top
        if (old_node == NULL) {//stack empty
            return -1;
        }
        new_node = old_node->next; //update new top to next node in stack
    } while (!atomic_compare_exchange_weak(&top, &old_node, new_node)); //assign new_node as top if top is still old_node
    id = old_node->node_id;
    free(old_node);
    return id;
}

/* the thread function */
void *thread_func(void *arg) {
    int opt = (int) (intptr_t) arg; //transform pointer to int
    int my_id = atomic_fetch_add(&thread_id, 1); // assign a unique ID to the thread

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

    /* Option 1: Mutex */
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
