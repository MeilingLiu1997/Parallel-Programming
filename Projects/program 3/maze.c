/* File:     maze.c
 * Purpose:  This program solves text-based mazes.
 *
 * Input:    A maze file to solve
 * Output:   Path that was taken to find the goal (end point) of the maze.
 *
 * Compile:  gcc -pthread -g -Wall maze.c -o maze
 *           (or run make)
 *
 * Run:      ./maze 01-small.txt
 *
 * ( ... Output of the maze as it's being explored ... )
 *
 * +-+-+-+-+-+-+-+-+-+-+
 * |     |.....|.......|
 * + +-+-+.+-+.+-+-+.+.+
 * |     |.| |.....|.|.|
 * + +-+ +-+ +-+-+.+.+.+
 * |   |         |.|.|.|
 * +-+ +-+-+-+-+ +.+-+.+
 * |   |         |.....|
 * + +-+ + +-+-+ +-+-+.+
 * | | | | |   | |   |.|
 * + + + +-+ + + + +-+.+
 * |   | |   | | | |...|
 * +-+ + + +-+ + + +.+-+
 * | | |   |   |   |.|.|
 * + + +-+-+ +-+-+ +.+.+
 * | | | |*  |...| |...|
 * + + + +.+ +.+.+-+-+.+
 * | | | |.| |.|.......|
 * + + + +.+-+.+-+-+-+-+
 * |     |............*|
 * +-+-+-+-+-+-+-+-+-+-+
 * Maze solved!
 * Time: 12.65
 * Goal location:
 * Index 418
 * Position (19, 19)
 * Threads created: 83
 */

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/* An array of characters that contain the maze data: 'S' indicates the starting
 * position of the maze and 'E' indicates the goal (ending position). Spaces
 * (' ') can be explored by the player, while any other character represents a
 * wall. */
char *maze;

/* Identical to the maze array, this character array contains the positions that
 * have already been explored by the player. */
char *search_locations;

/* This mutex protects access to the search_locations array to ensure multiple
 * threads don't modify it at the same time. */
pthread_mutex_t location_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int total;

/* Maze dimensions: */
int maze_width = 0;
int maze_height = 0;

/* Location of the goal ('E' character) AFTER being found by the player. Until
 * then, this variable is set to -1 to indicate the goal has not been found. */
int goal_index = -1;

/* Used to keep track of where the player was previously, and where they are
 * going next. */
struct exploration_log {
    int current_index;
    int last_index;
};

/* Function prototypes ------------------------------------------------------ */

double get_time();
bool load_maze(char *maze_file);
int find_start();

void index2position(int index, int *row, int *col);
int position2index(int row, int col);
int up_index(int index);
int down_index(int index);
int left_index(int index);
int right_index(int index);
bool is_explorable(int index);

void *view_thread(void *arg);
void *explore(void *input_ptr);

/* -------------------------------------------------------------------------- */

/* Function:  get_time
 * Purpose:   Retrieves the current UNIX timestamp as a double.
 *
 * Returns:   Current time in seconds.
 */
double get_time() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec / 1000000.0;
}

/* Function:  load_maze
 * Purpose:   Loads a maze file into memory.
 *
 * In args:   Name of the maze file to load.
 *
 * Returns:   false on failure, true on success.
 */
bool load_maze(char *maze_file) {
    int c;
    FILE *file = fopen(maze_file, "r");

    if (file == NULL) {
        perror("load_maze");
        return false;
    }

    printf("Reading maze: %s\n", maze_file);

    /* Determine the width of the maze: */
    while (fgetc(file) != '\n') {
        maze_width++;
    }

    /* Rewind to the start of the file */
    fseek(file, 0L, SEEK_SET);

    /* Determine the height of the maze: */
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            maze_height++;
        }
    }

    /* Rewind to the start of the file */
    fseek(file, 0L, SEEK_SET);

    printf("Dimensions: %dx%d\n", maze_width, maze_height);

    maze = malloc(maze_width * maze_height * sizeof(char));

    int i, j;
    for (i = 0; i < maze_height; ++i) {
        /* When j = maze_width, we're checking the last character (newline) */
        for (j = 0; j <= maze_width; ++j) {
            c = fgetc(file);
            if (c == '\n') {
                continue;
            }

            int index = position2index(i, j);
            maze[index] = (char) c;
        }
    }

    /* We'll also allocate an array to store the places we've explored: */
    search_locations = calloc(maze_width * maze_height, sizeof(char));

    return true;
}
int find_start(){
	int i, j, result;

	for (i = 0; i < maze_height; ++i) {
        for (j = 0; j < maze_width; ++j) {
            int index = position2index(i, j);
        	if(maze[index] == 'S'){
        		result = index;
        	}
        }
    }
	return result;
}

/* Function:  print_maze
 * Purpose:   Prints the current state of the maze to standard output.
 */
void print_maze() {
    int i, j;

    /* Since we can't clear the screen directly in C, we will just print 50
     * blank lines before printing the maze. */
    for (i = 0; i < 50; ++i) {
        puts("");
    }

    for (i = 0; i < maze_height; ++i) {
        for (j = 0; j < maze_width; ++j) {
            int index = position2index(i, j);
            if (search_locations[index] != 0) {
                /* If this location has already been searched, load character
                 * from the search_locations array. */
                putchar(search_locations[index]);
            } else {
                /* Otherwise, we'll load the character in the maze array. This
                 * is how we draw the walls, start, end, etc. */
                putchar(maze[index]);
            }
        }
        putchar('\n');
    }
}

/* Because we're using a 1D array to store our maze, we need a way to translate
 * from 1D array indices to 2D coordinates and back again. These two functions
 * help us accomplish this: */
void index2position(int index, int *row, int *col) {
    *row = index / maze_width;
    *col = index % maze_width;
}

int position2index(int row, int col) {
    return row * maze_width + col;
}

/* The following functions retrieve the positions above, below, to the left, and
 * to the right of the index passed in. */

int up_index(int index) {
    //TODO
    return index - maze_width;
}

int down_index(int index) {
    //TODO
    return index + maze_width;
}

int left_index(int index) {
    //TODO
    return index - 1;
}

int right_index(int index) {
    //TODO
    return index + 1;
}

//TODO function documentation
bool is_explorable(int index) {
    /* We can only walk on blank spaces (' ') or the goal ('E'). */
    return (maze[index] == ' ' || maze[index] == 'E');
}

//TODO function documentation
void *explore(void *input_ptr) {
    struct exploration_log *log = (struct exploration_log *) input_ptr;

    pthread_mutex_lock(&location_lock);
    search_locations[log->current_index] = '*';
    pthread_mutex_unlock(&location_lock);

    /* Whew! That was tiring. Let's sleep for a bit... */
    usleep(250000);

    if (maze[log->current_index] == 'E') {
        /* Found the goal! */
        goal_index = log->current_index;
        free(log);
        return 0;
    }

    if (goal_index >= 0) {
        /* Goal was already found, we don't need to continue */
        free(log);
        return 0;
    }

    int up = up_index(log->current_index);
    int down = down_index(log->current_index);
    int left = left_index(log->current_index);
    int right = right_index(log->current_index);

    struct exploration_log *new_log;

    //TODO We recursively call explore() handle branches in the maze. Update
    //this code to start new threads instead.

    pthread_t thread_handles;

    if (is_explorable(up) && up != log->last_index) {
        new_log = malloc(sizeof(struct exploration_log));
        new_log->last_index = log->current_index;
        new_log->current_index = up;
        //explore(new_log);
        pthread_create(&thread_handles, NULL, explore, new_log);
        pthread_mutex_lock(&mutex);
        total++;
        pthread_mutex_unlock(&mutex);
    }

    if (is_explorable(down) && down != log->last_index) {
        new_log = malloc(sizeof(struct exploration_log));
        new_log->last_index = log->current_index;
        new_log->current_index = down;
        //explore(new_log);
        pthread_create(&thread_handles, NULL, explore, new_log);
        pthread_mutex_lock(&mutex);
        total++;
        pthread_mutex_unlock(&mutex);
    }

    if (is_explorable(left) && left != log->last_index) {
        new_log = malloc(sizeof(struct exploration_log));
        new_log->last_index = log->current_index;
        new_log->current_index = left;
        //explore(new_log);
        pthread_create(&thread_handles, NULL, explore, new_log);
        pthread_mutex_lock(&mutex);
        total++;
        pthread_mutex_unlock(&mutex);
    }

    if (is_explorable(right) && right != log->last_index) {
        new_log = malloc(sizeof(struct exploration_log));
        new_log->last_index = log->current_index;
        new_log->current_index = right;
        //explore(new_log);
        pthread_create(&thread_handles, NULL, explore, new_log);
        pthread_mutex_lock(&mutex);
        total++;
        pthread_mutex_unlock(&mutex);
    }

    pthread_mutex_lock(&location_lock);
    search_locations[log->current_index] = '.';
    pthread_mutex_unlock(&location_lock);

    free(log);
    return 0;
}

//TODO function documentation
void *view_thread(void *arg) {
    /* While the goal hasn't been reached yet, keep printing out the state of
     * the maze: */
    while (goal_index < 0) {
        /* usleep() is like sleep(), but with microsecond precision. We will
         * wait 0.25 seconds between each printout. If we were to let this loop
         * go without sleeping, the thread would consume an excessive amount of
         * CPU and update too frequently. Similar behavior is seen in games:
         * rather than updating the screen as fast as possible, it's better to
         * sync with the display refresh rate (commonly 60 Hz -- 60 times a
         * second) and only update that often. */
        usleep(250000);

        pthread_mutex_lock(&location_lock);
        print_maze();
        pthread_mutex_unlock(&location_lock);
    }

     return NULL;
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        printf("usage: %s <maze_file.txt>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (load_maze(argv[1]) == false) {
        printf("Failed to load maze file!\n");
        return EXIT_FAILURE;
    }

    //TODO we have hard-coded the location of the starting point. You should
    //write a function that will determine this programmatically. Hint: take a
    //look at how we print the maze to see how to iterate through it.
    int row, col;
    index2position(find_start(), &row, &col);
    int start = position2index(row, col);

    /* Start our view thread. This thread continuously draws the state of the
     * maze (player location, explored locations, and the maze walls) so we can
     * see its updates in real time. We don't need to pass an argument to the
     * thread, so we provide NULL here. */
    pthread_t viewer;
    pthread_create(&viewer, NULL, view_thread, NULL);

    /* Let's start exploring the maze. */
    struct exploration_log *log = malloc(sizeof(struct exploration_log));
    log->current_index = start;
    log->last_index = -1;

    double begin = get_time();
    explore(log);

    pthread_join(viewer, NULL);

    double end = get_time();
    double elapsed = end - begin;


    /* We're finished! If we found the goal, print out some info here: */
    if (goal_index >= 0) {
        printf("Maze solved!\n");
        printf("Time: %.2f seconds\n", elapsed);
        int x, y;
        index2position(goal_index, &x, &y);
        printf("Goal location: \nIndex %d\nPosition (%d, %d)\n",
                goal_index, x, y);
        printf("Threads created: %d\n", total);
    }

    return 0;
}
