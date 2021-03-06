/*
Name : Syed Talha
BlazerID: smtalha
Course section: 632
Homework #: 3 (2D Data Distribution)
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <omp.h>

//function signatures
int** create_int_matrix(int rows, int columns);
void initialize_matrix(int** m, int rows, int columns);
void populate_ghost_cells(int** m, int rows, int columns);
int count_alive_neighbors(int** m, int row, int column);
void print_int_matrix(int** m, int rows, int columns);
void free_int_matrix(int** m, int rows);
int start_generations(int** board, int boardSize, int max_iterations, int grid_size_x, int grid_size_y);
double gettime();
void write_to_file(char * fileName, char * str);

int main(int argc, char * argv[]) {
    if(argc < 5) {
        printf("Please provide [board-size], [max-generations], [grid-size-x] and [grid-size-y] as command-line arguments.\n");
        return 1;
    }

    int boardSize = atoi(argv[1]);
    int maxGenerations = atoi(argv[2]);
    int gridSizeX = atoi(argv[3]);
    int gridSizeY = atoi(argv[4]);

    double starttime, endtime;

    starttime = gettime();

    int** board = create_int_matrix(boardSize + 2, boardSize + 2);

    if(board == NULL) {
        printf("Unable to allocate memory.\n");
        return 1;
    }

    initialize_matrix(board, boardSize, boardSize);

    populate_ghost_cells(board, boardSize, boardSize);

    //printf("Initial Board:\n\n");
    //print_int_matrix(board, boardSize + 2, boardSize + 2);

    int actualNumOfGenerations = start_generations(board, boardSize, maxGenerations, gridSizeX, gridSizeY);

    free_int_matrix(board, boardSize + 2);

    endtime = gettime();

    double seconds = endtime-starttime;
    double minutes = seconds / 60.0;
    double hours = minutes / 60.0;

    char output[300];
    sprintf(output, "Board Size: %d\nMax Generations: %d\nActual Number of Generations: %d\nGrid Size: %d x %d\n\nTime taken = %lf seconds or %lf minutes or %lf hours.\n\n", boardSize, maxGenerations, actualNumOfGenerations, gridSizeX, gridSizeY, seconds, minutes, hours);
    write_to_file("output.txt", output);
    //printf("\nTime taken = %lf seconds or %lf minutes or %lf hours.\nGenerations: %d\nGrid Size: %d x %d\n", seconds, minutes, hours, actualNumOfGenerations, gridSizeX, gridSizeY);

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

int start_generations(int** board, int boardSize, int max_iterations, int grid_size_x, int grid_size_y) {
    int flag = 1;
    int numOfIterations;

    int i, j;

    int numOfAliveNeighbors;

    #pragma omp parallel num_threads(grid_size_x) private(i, numOfIterations)
    for(numOfIterations = 0; numOfIterations < max_iterations && flag != 0; numOfIterations++) {
        flag = 0;
       
        #pragma omp for
        for(i = 1; i <= boardSize; i++) {
            #pragma omp parallel for num_threads(grid_size_y) private(j, numOfAliveNeighbors)
            for(j = 1; j <= boardSize; j++) {

                numOfAliveNeighbors = count_alive_neighbors(board, i, j);
                
                if(board[i][j] == 1) {//if the cell is alive                    
                    if(numOfAliveNeighbors != 2 && numOfAliveNeighbors != 3) {
                        board[i][j] = 0;
                        flag++;
                    }
                } else {//if the cell is dead                    
                    if(numOfAliveNeighbors == 3) {
                        board[i][j] = 1;
                        flag++;
                    }
                }
                //printf("Generation Y %d: Thread: %d\n\n", numOfIterations, omp_get_thread_num());
            }
            //printf("Generation X %d: Thread: %d\n\n", numOfIterations, omp_get_thread_num());
        }

        //Comment the following print statements for larger board sizes
        //printf("Generation %d: Thread: %d\n\n", numOfIterations, omp_get_thread_num());
        //printf("Flag: %d\n\n", flag);
        //print_int_matrix(board, boardSize + 2, boardSize + 2);
    }

    return numOfIterations;
}

double gettime() {
  struct timeval tval;

  gettimeofday(&tval, NULL);

  return( (double)tval.tv_sec + (double)tval.tv_usec/1000000.0 );
}

void write_to_file(char * fileName, char * str) {
    FILE * fp = fopen(fileName, "a");
    fprintf(fp, str);
    fclose(fp);
}