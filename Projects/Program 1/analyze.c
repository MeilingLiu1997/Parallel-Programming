/* File:     analyze.c
 * Purpose:  Perform analysis on US Census data.
 *
 * Input:    CSV File(s) to analyze.
 * Output:   Summary information about the data.
 *
 * Compile:  gcc -g -Wall -o analyze analyze.c
 *           (or run make)
 *
 * Run:      ./analyze data_mt.csv data_ne.csv
 *
 * Opening file: data_mt.csv
 * Opening file: data_ne.csv
 * States found: MT NE
 * -- State: MT --
 * Number of Records: 22807
 * Sample Population: 46114
 * Average Income: 35222.89
 * Average Rent: 520.18
 * Average vehicles per household: 1.80
 * Vehicles per capita: 0.89
 * -- State: NE --
 * Number of Records: 41129
 * Sample Population: 89113
 * Average Income: 41192.08
 * Average Rent: 507.69
 * Average vehicles per household: 1.85
 * Vehicles per capita: 0.85
 *
 * CSV format:
 *
 (Revised)
 ID, code, people, income, vehicles, rent

    31, NE, 1, 0, 1, 0
    31, NE, 02, 00252000, 2, 0
    31, NE, 02, 00014000, 1, 00210
    31, NE, 04, 00069000, 4, 0
    31, NE, 1, 0, 1, 0
    31, NE, 04, 00042000, 1, 00450
    31, NE, 02, 00067700, 2, 0
 *
 * Fields:
 *      State ID,
 *      state code,
 *      people in household,
 *      income,
 *      vehicles,
 *      rent
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TODO: Add elements to the state_info struct as necessary. For instance, you
 * may want to determine the sample population by recording the total number of
 * people found in each household */
struct state_info {
    int state_id;
    char code[3];
    unsigned long num_records;
    unsigned long people_in_household;
    unsigned long total_income;
    unsigned long total_vehicles;
    unsigned long total_rent;
    unsigned long renters;
};

void analyze_file(FILE *file, struct state_info *states[], int num_states);
void print_report(struct state_info *states[], int num_states);

int main(int argc, char *argv[]) {

    /* TODO: fix this conditional. You should be able to read multiple files. */
    if (argc <= 1){
        printf("Usage: %s csv_file1 csv_file2 ... csv_fileN \n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Let's create an array to store our state data in. As we know, there are
     * 50 states. However, the US Census also includes unincorporated
     * territories such as Puerto Rico. We'll use 100 to be safe. */
    const int num_states = 100;
    struct state_info *states[num_states] = { NULL };

    for (int i = 1; i < argc; ++i) {
        /* TODO: Open the file for reading */
        printf("Opening file: %s\n", argv[i]); 
        FILE *file = fopen(argv[i], "r");

        /* TODO: If the file doesn't exist, print an error message and move on
         * to the next file. */
        if (file == NULL) {
            perror("fopen");
            return EXIT_FAILURE; 
        }
        /* TODO: Analyze the file */
        analyze_file(file, states, num_states);
        fclose(file);
    }

    /* Now that we have recorded data for each file, we'll summarize them: */
    print_report(states, num_states);

    /* free */
	for (int i = 0; i < num_states; ++i) {
        if (states[i] != NULL) {
        	free(states[i]);
        }
    }
    return 0;
}

/* TODO function documentation. Follow the format from previous assignments.*/
void analyze_file(FILE *file, struct state_info *states[], int num_states) {
    const int line_sz = 100;
    char line[line_sz];
    
    while (fgets(line, line_sz, file) != NULL) {
        int not_rent = 0;
        /* TODO: tokenize the line */
        char *token = strtok(line, ", \n");
        /* TODO: We need to do a few things here:
         *
         *       * Determine what state this line is for
         *       * If our states array doesn't have a state_info entry for this
         *         state, then we need to allocate memory for it and put it in
         *         the array. Otherwise, we reuse the existing entry.
         *       * Update the structure as necessary
         */

        int id = atoi(token);
        struct state_info *state = states[id];
        if (states[id] == NULL){
            state = calloc(1, sizeof(struct state_info));
            states[id] = state;
        }
        state->state_id = id;                                   // put id

        state->num_records++;                // put number of records

        token = strtok(NULL, ", \n");
        strcpy(state->code, token);                             // put code

        token = strtok(NULL, ", \n");
        int people_in_household = atoi(token);
        state->people_in_household += people_in_household;                // put people in household
        
        token = strtok(NULL, ", \n");
        int income = atol(token);
        state->total_income += income;                            // put total income

        token = strtok(NULL, ", \n");
        int vehicles = atol(token);
        state->total_vehicles += vehicles;                       // put total vehicles

        token = strtok(NULL, ", \n");
        int rent = atoi(token);
        state->total_rent += rent;                                     // put rent

        if(rent != 0){
            state->renters++;
        }
    }
}

/* TODO function documentation. Follow the format from previous assignments.*/
void print_report(struct state_info *states[], int num_states) {
    printf("States found: ");
    for (int i = 0; i < num_states; ++i) {
        if (states[i] != NULL) {
            struct state_info *info = states[i];
            printf("%s ", info->code);
        }
    }
    printf("\n"); 
    for (int i = 0; i < num_states; ++i) {
        if (states[i] != NULL) {
            struct state_info *info = states[i];

            /* TODO: Print out the summary for each state */
            printf("-- State: %s --\n", info->code);
            printf("Number of Records: %lu\n", info->num_records);
            printf("Sample Population:: %lu\n", info->people_in_household);
            printf("Average Income: %.2f\n", (float)(info->total_income)/(info->num_records));
            printf("Average Rent: %.2f\n", (float)(info->total_rent)/(info->renters));
            printf("Average vehicles per household: %.2f\n", (float)(info->total_vehicles)/(info->num_records));
            printf("Vehicles per capita: %.2f\n", (float)(info->total_vehicles)/(info->people_in_household));
        }

    }

    printf("\n");
}
