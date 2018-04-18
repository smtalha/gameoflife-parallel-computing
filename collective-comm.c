/*
Name : Syed Talha
BlazerID: smtalha
Course section: 632
Homework #: 5
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <mpi.h>

#define DEBUG 1

//function prototypes
void write_to_file(char * file_name, char * str);
int* create_int_array(int size);
void initialize_int_array(int* arr, int size);
void linear(int rank, int num_tasks, int* arr, int size);
void ring(int rank, int num_tasks, int* arr, int size);
void double_ring(int rank, int num_tasks, int* arr, int size);
void tree(int rank, int num_tasks, int* arr, int size);
double reduce_max_double(double value, int rank, int num_tasks);

int main(int argc, char * argv[]) {
    if(argc < 2) {
        printf("Please provide [communication-type] as command-line argument.\n\n1. Linear\n2. Ring\n3. Double Ring\n4. Tree");
        return 1;
    }

    int comm_type = atoi(argv[1]);

    if(comm_type < 1 || comm_type > 4) {
        printf("Invalid communication type.\n\n1. Linear\n2. Ring\n3. Double Ring\n4. Tree");
        return 1;
    }

    int num_tasks, rank;
    
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    double i;
    double exp = 5.0;
    double max_exp = 20.0;
    for(i = pow(2.0, exp); i <= pow(2.0, max_exp); i*=2) {
        int size = (int)i / sizeof(int);
        int* arr = create_int_array(size);

        if(rank == 0) {
            initialize_int_array(arr, size);
        }

        switch(comm_type) {
            case 1:
                linear(rank, num_tasks, arr, size);
                break;
            case 2:
                ring(rank, num_tasks, arr, size);
                break;
            case 3:
                double_ring(rank, num_tasks, arr, size);
                break;
            case 4:
                tree(rank, num_tasks, arr, size);
                break;
            default:
                break;
        }
        exp++;
        free(arr);
    }
    
    MPI_Finalize();

    return 0;
}

void linear(int rank, int num_tasks, int* arr, int size) {
    MPI_Status status;
    double local_start, local_finish, local_elapsed, elapsed;

    MPI_Barrier(MPI_COMM_WORLD);

    local_start = MPI_Wtime();

    if(rank == 0) {
        int i;
        for(i = 1; i < num_tasks; i++) {
           MPI_Send(arr, size, MPI_INT, i, 0, MPI_COMM_WORLD); 
        }        
    } else {
        MPI_Recv(arr, size, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        if(DEBUG) {
            printf("Rank %d ----> Rank %d.\n", status.MPI_SOURCE, rank);
        }
    }

    local_finish = MPI_Wtime();
    local_elapsed = local_finish - local_start;

    elapsed = reduce_max_double(local_elapsed, rank, num_tasks);

    if(rank == 0) {
        if(DEBUG) {
            //Print output to console
            printf("Processes: %d\tMessage Size:%8d\tTime taken = %lf seconds.\n", num_tasks, size*sizeof(int), elapsed);
        } else {
            //Write output to file
            char output[300];
            sprintf(output, "Processes: %d\tMessage Size: %8d\tTime taken = %lf seconds.\n", num_tasks, size*sizeof(int), elapsed);
            write_to_file("output.txt", output);
        }
    }
}

void ring(int rank, int num_tasks, int* arr, int size) {
    MPI_Status status;
    double local_start, local_finish, local_elapsed, elapsed;

    MPI_Barrier(MPI_COMM_WORLD);

    local_start = MPI_Wtime();

    int prev = rank - 1;
    int next = rank + 1;

    if(rank == 0) {
        MPI_Send(arr, size, MPI_INT, next, 0, MPI_COMM_WORLD); 
    } else if (rank == num_tasks - 1) {
        MPI_Recv(arr, size, MPI_INT, prev, 0, MPI_COMM_WORLD, &status);
        if(DEBUG) {
            printf("Rank %d ----> Rank %d.\n", status.MPI_SOURCE, rank);
        }
    } else {
        MPI_Recv(arr, size, MPI_INT, prev, 0, MPI_COMM_WORLD, &status);
        if(DEBUG) {
            printf("Rank %d ----> Rank %d.\n", status.MPI_SOURCE, rank);
        }
        MPI_Send(arr, size, MPI_INT, next, 0, MPI_COMM_WORLD);
    }

    local_finish = MPI_Wtime();
    local_elapsed = local_finish - local_start;

    elapsed = reduce_max_double(local_elapsed, rank, num_tasks);

    if(rank == 0) {
        if(DEBUG) {
            //Print output to console
            printf("Processes: %d\tMessage Size:%8d\tTime taken = %lf seconds.\n", num_tasks, size*sizeof(int), elapsed);
        } else {
            //Write output to file
            char output[300];
            sprintf(output, "Processes: %d\tMessage Size: %8d\tTime taken = %lf seconds.\n", num_tasks, size*sizeof(int), elapsed);
            write_to_file("output.txt", output);
        }
    }
}

void double_ring(int rank, int num_tasks, int* arr, int size) {
    MPI_Status status;
    double local_start, local_finish, local_elapsed, elapsed;

    MPI_Barrier(MPI_COMM_WORLD);

    local_start = MPI_Wtime();

    int middle = num_tasks / 2;

    int prev = rank - 1;
    int next = rank + 1;

    if(rank <= middle) {
        if(rank == 0) {
            MPI_Send(arr, size, MPI_INT, num_tasks - 1, 0, MPI_COMM_WORLD);  
        }
        if(rank != middle) {
            MPI_Send(arr, size, MPI_INT, next, 1, MPI_COMM_WORLD);
        }
        if(rank != 0) {
            MPI_Recv(arr, size, MPI_INT, prev, 1, MPI_COMM_WORLD, &status);
            if(DEBUG) {
                printf("Rank %d ----> Rank %d.\n", status.MPI_SOURCE, rank);
            }
        }
    } else {        
        if(rank == num_tasks - 1) {
            MPI_Recv(arr, size, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            if(DEBUG) {
                printf("Rank %d ----> Rank %d.\n", status.MPI_SOURCE, rank);
            }
        }
        if(rank != middle + 1) {
            MPI_Send(arr, size, MPI_INT, prev, 2, MPI_COMM_WORLD);
        }
        if(rank != num_tasks - 1) {
            MPI_Recv(arr, size, MPI_INT, next, 2, MPI_COMM_WORLD, &status);
            if(DEBUG) {
                printf("Rank %d ----> Rank %d.\n", status.MPI_SOURCE, rank);
            }
        }
    }

    local_finish = MPI_Wtime();
    local_elapsed = local_finish - local_start;

    elapsed = reduce_max_double(local_elapsed, rank, num_tasks);

    if(rank == 0) {
        if(DEBUG) {
            //Print output to console
            printf("Processes: %d\tMessage Size:%8d\tTime taken = %lf seconds.\n", num_tasks, size*sizeof(int), elapsed);
        } else {
            //Write output to file
            char output[300];
            sprintf(output, "Processes: %d\tMessage Size: %8d\tTime taken = %lf seconds.\n", num_tasks, size*sizeof(int), elapsed);
            write_to_file("output.txt", output);
        }
    }
}

void tree(int rank, int num_tasks, int* arr, int size) {
    MPI_Status status;
    double local_start, local_finish, local_elapsed, elapsed;

    MPI_Barrier(MPI_COMM_WORLD);

    local_start = MPI_Wtime();

    int target;
    unsigned btmsk = 1;
    int join = btmsk << 1;

    while (btmsk < num_tasks) {
        if (rank < join) {
            target = rank ^ btmsk;
            if (rank < target) {
                if (target < num_tasks )
                    MPI_Send(arr, size, MPI_INT, target, 0, MPI_COMM_WORLD);
            } else {
                MPI_Recv(arr, size, MPI_INT, target, 0, MPI_COMM_WORLD, &status);
                if(DEBUG) {
                    printf("Rank %d ----> Rank %d.\n", status.MPI_SOURCE, rank);
                } 
            }
        }
        btmsk <<= 1;
        join <<= 1;
    }

    local_finish = MPI_Wtime();
    local_elapsed = local_finish - local_start;

    elapsed = reduce_max_double(local_elapsed, rank, num_tasks);

    if(rank == 0) {
        if(DEBUG) {
            //Print output to console
            printf("Processes: %d\tMessage Size:%8d\tTime taken = %lf seconds.\n", num_tasks, size*sizeof(int), elapsed);
        } else {
            //Write output to file
            char output[300];
            sprintf(output, "Processes: %d\tMessage Size: %8d\tTime taken = %lf seconds.\n", num_tasks, size*sizeof(int), elapsed);
            write_to_file("output.txt", output);
        }
    }
}

double reduce_max_double(double value, int rank, int num_tasks) {
    double max = value;
    double temp;
    int peer;
    int finished = 0;
    unsigned btmsk = (unsigned) 1;

    while (!finished && btmsk < num_tasks) {
        peer = rank ^ btmsk;
        if (rank < peer) {
            if (peer < num_tasks) {
                MPI_Recv(&temp, 1, MPI_DOUBLE, peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if(temp > max) {
                    max = temp;
                }
            }
            btmsk <<= 1;
        } else {
            MPI_Send(&max, 1, MPI_DOUBLE, peer, 0, MPI_COMM_WORLD); 
            finished = 1;
        }
    }

    return max;
}

int* create_int_array(int size) {
    //dynamicaly create a size length int array
    int* arr = malloc(size * sizeof(int));

    if(arr == NULL) {
        printf("Unable to allocate memory.\n");
        return NULL;
    }

    return arr;
}

void initialize_int_array(int* arr, int size) {
    int i;
    for(i = 0; i < size; i++) {
        arr[i] = 0;
    }
}

void write_to_file(char * file_name, char * str) {
    FILE * fp = fopen(file_name, "a"/* append */);
    fprintf(fp, str);
    fclose(fp);
}