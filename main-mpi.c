/*
Name : Syed Talha
BlazerID: smtalha
Course section: 632
Homework #: 4 
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <mpi.h>

//function signatures
int** create_int_matrix(int rows, int columns);
void initialize_matrix(int** m, int rows, int columns);
void populate_ghost_cells(int** m, int rows, int columns);
int count_alive_neighbors(int** m, int row, int column);
void print_int_matrix(int** m, int rows, int columns);
void free_int_matrix(int** m, int rows);
int compute_generation(int** board, int** temp, int board_size, int local_a, int local_b);
void write_to_file(char * file_name, char * str);

int main(int argc, char * argv[]) {
    if(argc < 3) {
        printf("Please provide [board-size] and [max-generations] as command-line arguments.\n");
        return 1;
    }

    int board_size = atoi(argv[1]);
    int max_generations = atoi(argv[2]);

    if(board_size < 3) {
        printf("Board size too small.\n");
        return 1;
    }

    int num_tasks, rank;
    
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int local_n = board_size / num_tasks;
    int local_a = (rank * local_n) + 1;
    int local_b = local_a + local_n - 1;

    if (rank == num_tasks - 1 && board_size % num_tasks != 0) {
        local_b++;
    }

    //printf("local_n = %d\n", local_n);
    //printf("local_a = %d (rank: %d)\n", local_a, rank);
    //printf("local_b = %d (rank: %d)\n", local_b, rank);

    // determine left and right neighbors
    int prev = rank - 1;
    int next = rank + 1;
    if (rank == 0)  prev = num_tasks - 1;
    if (rank == (num_tasks - 1))  next = 0;

    MPI_Request reqs[4];
    MPI_Status stats[4];

    int** board = create_int_matrix(board_size + 2, board_size + 2);
    int** temp = create_int_matrix(board_size + 2, board_size + 2);

    if(board == NULL || temp == NULL) {
        return 1;
    }

    initialize_matrix(board, board_size, board_size);
    populate_ghost_cells(board, board_size, board_size);
    
    //printf("Initial Board: (rank = %d)\n\n", rank);
    //print_int_matrix(board, board_size + 2, board_size + 2);
    
    int** ptr;
    
    int flag = 1;
    int num_iterations;

    double local_start, local_finish, local_elapsed, elapsed;

    MPI_Barrier(MPI_COMM_WORLD);

    local_start = MPI_Wtime();

    for(num_iterations = 0; num_iterations < max_generations && flag != 0; num_iterations++) {
       
        flag = compute_generation(board, temp, board_size, local_a, local_b);

        ptr = board;
        board = temp;
        temp = ptr;

        MPI_Irecv(board[local_b + 1], board_size + 2, MPI_INT, next, 1, MPI_COMM_WORLD, &reqs[0]);
        MPI_Irecv(board[local_a - 1], board_size + 2, MPI_INT, prev, 2, MPI_COMM_WORLD, &reqs[1]);

        MPI_Isend(board[local_b], board_size + 2, MPI_INT, next, 2, MPI_COMM_WORLD, &reqs[2]);
        MPI_Isend(board[local_a], board_size + 2, MPI_INT, prev, 1, MPI_COMM_WORLD, &reqs[3]);

        MPI_Waitall(4, reqs, stats);

        //printf("Generation %d: (rank = %d)\n\n", num_iterations, rank);
        //print_int_matrix(board, board_size + 2, board_size + 2);
    }

    local_finish = MPI_Wtime();
    local_elapsed = local_finish - local_start;

    MPI_Reduce(&local_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    double seconds = elapsed;
    double minutes = seconds / 60.0;
    double hours = minutes / 60.0;

    if (rank == 0) {
        char output[300];
        sprintf(output, "Board Size: %d\nMax Generations: %d\nActual Number of Generations: %d\nNumber of processes: %d\n\nTime taken = %lf seconds or %lf minutes or %lf hours.\n\n", board_size, max_generations, num_iterations, num_tasks, seconds, minutes, hours);
        write_to_file("output.txt", output);
        //printf("\nTime taken = %lf seconds or %lf minutes or %lf hours.\nGenerations: %d\n", seconds, minutes, hours, num_iterations);
    }

    free_int_matrix(board, board_size + 2);
    free_int_matrix(temp, board_size + 2);

    MPI_Finalize();

    return 0;
}

int** create_int_matrix(int rows, int columns) {
    //dynamicaly create a [rows x columns] matrix
    int** matrix = malloc(rows * sizeof(int*));

    if(matrix == NULL) {
        printf("Unable to allocate memory.\n");
        return NULL;
    }

    int i;
    for(i = 0; i < rows; i++) {
        matrix[i] = malloc(columns * sizeof(int));
        if(matrix[i] == NULL) {
            printf("Unable to allocate memory.\n");
            return NULL;
        }
    }

    return matrix;
}

void initialize_matrix(int** m, int rows, int columns) {
    //randomly initialize matrix
    int i, j;
    for(i = 1; i <= rows; i++) {
        srand(54321|i);
        for(j = 1; j <= columns; j++) {
            if(drand48() < 0.5) {
                m[i][j] = 0;
            } else {
                m[i][j] = 1;
            }
        }
    }
}

void populate_ghost_cells(int** m, int rows, int columns) {
    int i;

    //populate first row
    for(i = 1; i <= columns; i++) {
        m[0][i] = m[rows][i];
    }

    //populate last row
    for(i = 1; i <= columns; i++) {
        m[rows+1][i] = m[1][i];
    }

    //populate first column
    for(i = 0; i <= rows+1; i++) {
        m[i][0] = m[i][columns];
    }

    //populate last column
    for(i = 0; i <= rows+1; i++) {
        m[i][columns+1] = m[i][1];
    }
}

int count_alive_neighbors(int** m, int row, int column) {
    //count alive neighbors of a particular cell
    int count = 0;

    count += m[row][column-1];
    count += m[row][column+1];
    count += m[row-1][column];
    count += m[row+1][column];
    count += m[row-1][column-1];
    count += m[row-1][column+1];
    count += m[row+1][column-1];
    count += m[row+1][column+1];

    return count;
}

void print_int_matrix(int** m, int rows, int columns) {
    int i, j;
    for(i = 0; i < rows; i++) {
        for(j = 0; j < columns; j++) {
            printf("%d ", m[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void free_int_matrix(int** m, int rows) {
    int i;
    for(i = 0; i < rows; i++) {
        free(m[i]);
    }
    free(m);
}

int compute_generation(int** board, int** temp, int board_size, int local_a, int local_b) {
    int flag = 0;
    int num_alive_neighbors;

    int i, j;
    
    for(i = local_a; i <= local_b; i++) {
        for(j = 1; j <= board_size; j++) {
            num_alive_neighbors = count_alive_neighbors(board, i, j);
            
            if(board[i][j] == 1) {//if the cell is alive                    
                if(num_alive_neighbors != 2 && num_alive_neighbors != 3) {
                    temp[i][j] = 0;
                    flag++;
                }
            } else {//if the cell is dead                    
                if(num_alive_neighbors == 3) {
                    temp[i][j] = 1;
                    flag++;
                }
            }
        }
    }

    return flag;
}

void write_to_file(char * file_name, char * str) {
    FILE * fp = fopen(file_name, "a");
    fprintf(fp, str);
    fclose(fp);
}