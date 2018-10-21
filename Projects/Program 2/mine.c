/* File:     mine.c
 * Purpose:  Demonstrates the hash inversion technique used by cryptocurrencies
 *           such as bitcoin.
 *
 *           NOTE: This program includes an implementation of the MD5 algorithm
 *           by Tim Caswell, source:
 *               https://gist.github.com/creationix/4710780
 *
 * Input:    Block difficulty and contents (as a string)
 * Output:   Hash inversion solution (nonce) and timing statistics.
 *
 * Compile:  mpicc -g -Wall -lm mine.c -o mine
 *           (or run make)
 *           When your code is ready for performance testing, you can add the
 *           -O3 flag to enable all compiler optimizations.
 *
 * Run:      mpirun -n 4 ./mine 24 'Hello CS 220!!!'
 *
 * Difficulty Mask: 00000000000000000000000011111111
 * Number of processes: 4
 * Solution found by rank 2 (jet01):
 * Nonce: 9223372036876236423
 * Hash: 0000007f639022687a59b427c3a0edeb
 * 83087328 hashes in 8.46s (9817225.00 hashes/sec)
 */

#include <limits.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

uint32_t swap_endian(uint32_t num);
void md5(char *initial_msg, size_t initial_len);
void mine(char *block, uint32_t difficulty_mask,
        uint64_t nonce_start, uint64_t nonce_end,
        uint64_t *nonce_out, uint64_t *num_inversions);
void print_binary32(uint32_t num);
void print_hash(void);

// leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))

// These vars will contain the hash
uint32_t h0, h1, h2, h3;

/* Function:  md5
 * Purpose:   Computes MD5 hash.
 *
 * In args:   initial_msg: string to compute the hash code for.
 *            initial_len: length of the string.
 *
 * Note:      The output of the hashing process is stored in h0, h1, h2, and h3.
 *
 *            ** You do NOT need to modify the following function! **
 */
void md5(char *initial_msg, size_t initial_len) {

    // Message (to prepare)
    uint8_t *msg = NULL;

    // Note: All variables are unsigned 32 bit and wrap modulo 2^32 when
    // calculating

    // r specifies the per-round shift amounts

    uint32_t r[] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
        5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
        6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

    // Use binary integer part of the sines of integers (in radians) as
    // constants
    // Initialize variables:
    uint32_t k[] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
        0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
        0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
        0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
        0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
        0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
        0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
        0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

    h0 = 0x67452301;
    h1 = 0xefcdab89;
    h2 = 0x98badcfe;
    h3 = 0x10325476;

    // Pre-processing: adding a single 1 bit
    //append "1" bit to message    
    /* Notice: the input bytes are considered as bits strings,
       where the first bit is the most significant bit of the byte.[37] */

    // Pre-processing: padding with zeros
    //append "0" bit until message length in bit ≡ 448 (mod 512)
    //append length mod (2 pow 64) to message

    int new_len;
    for(new_len = initial_len*8 + 1; new_len%512!=448; new_len++);
    new_len /= 8;

    msg = calloc(new_len + 64, 1); // also appends "0" bits 
    // (we alloc also 64 extra bytes...)
    memcpy(msg, initial_msg, initial_len);
    msg[initial_len] = 128; // write the "1" bit

    uint32_t bits_len = 8*initial_len; // note, we append the len
    memcpy(msg + new_len, &bits_len, 4); // in bits at the end of the buffer

    // Process the message in successive 512-bit chunks:
    //for each 512-bit chunk of message:
    int offset;
    for(offset=0; offset<new_len; offset += (512/8)) {

        // break chunk into sixteen 32-bit words w[j], 0 ≤ j ≤ 15
        uint32_t *w = (uint32_t *) (msg + offset);

        // Initialize hash value for this chunk:
        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;

        // Main loop:
        uint32_t i;
        for(i = 0; i<64; i++) {

            uint32_t f, g;

            if (i < 16) {
                f = (b & c) | ((~b) & d);
                g = i;
            } else if (i < 32) {
                f = (d & b) | ((~d) & c);
                g = (5*i + 1) % 16;
            } else if (i < 48) {
                f = b ^ c ^ d;
                g = (3*i + 5) % 16;
            } else {
                f = c ^ (b | (~d));
                g = (7*i) % 16;
            }

            uint32_t temp = d;
            d = c;
            c = b;
            b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
            a = temp;
        }
        // Add this chunk's hash to result so far:
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
    }
    // cleanup
    free(msg);
}

/* Function:  print_hash
 * Purpose:   Prints the current MD5 hash code in hexadecimal.
 */
void print_hash(void) {
    uint8_t *p;
    p = (uint8_t *) &h0;
    printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);

    p = (uint8_t *) &h1;
    printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);

    p = (uint8_t *) &h2;
    printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);

    p = (uint8_t *) &h3;
    printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
    puts("");
}

/* Function:  print_binary32
 * Purpose:   Prints a 32-bit binary number.
 *
 * In args:   num: Number to print
 */
void print_binary32(uint32_t num) {
    int i;
    for (i = 31; i >= 0; --i) {
        uint32_t position = 1 << i;
        printf("%c", ((num & position) == position) ? '1' : '0');
    }
}

/* Function:  swap_endian
 * Purpose:   Changes the byte ordering of a number.
 *
 * In args:   num: Number to swap
 *
 * Returns:   The number with swapped byte ordering
 */
uint32_t swap_endian(uint32_t num) {
    uint32_t byte0, byte1, byte2, byte3;

    byte0 = (num & 0x000000ff) << 24u;
    byte1 = (num & 0x0000ff00) << 8u;
    byte2 = (num & 0x00ff0000) >> 8u;
    byte3 = (num & 0xff000000) >> 24u;

    return byte0 | byte1 | byte2 | byte3;
}

/* Function:  mine
 * Purpose:   Performs hash inversions to discover a nonce (number only used
 *            once) that will produce a hash code with the desired difficulty
 *            level (number of leading zeroes).
 *
 * In args:   block: Block data to hash
 *            difficulty_mask: A bit mask with the number of leading zeroes
 *            nonce_start: Nonce to start with (first number to try)
 *            nonce_end: Last nonce to try
 *
 * Out args:  nonce_out: If found, the nonce that solves the block
 *            num_inversions: The number of hash inversions performed by mine()
 */
void mine(
        char *block,
        uint32_t difficulty_mask,
        uint64_t nonce_start,			// 0
        uint64_t nonce_end,				// MAX
        uint64_t *nonce_out,			// 0
        uint64_t *num_inversions) {		// hash = 0

    uint64_t nonce;
    for (nonce = nonce_start; nonce < nonce_end; ++nonce) {
        /* Convert the nonce to a string */
        char buf[32];
        sprintf(buf, "%llu", nonce);

        /* Create a new string by concatenating the block and nonce string. For
         * example, if we have 'Hello World!' and '10', the new string is:
         * 'Hello World!10' */
        size_t str_size = strlen(buf) + strlen(block);
        char *tmp_str = malloc(sizeof(char) * str_size + 1);
        strcpy(tmp_str, block);
        strcat(tmp_str, buf);

        /* Hash the combined string */
        md5(tmp_str, strlen(tmp_str));

        /* Clean up the temporary string */
        free(tmp_str);

        /* Check to see if we've found a solution to our block */
        uint32_t hash_front = swap_endian(h0);
        if ((hash_front & difficulty_mask) == hash_front) {
            *nonce_out = nonce;
            *num_inversions = nonce - nonce_start;
            return;
        }

        // TODO before moving on to the next hash inversion, we should probably
        // check to see if we got a message from another process saying that it
        // already found a solution.
        int flag = 0;	
		MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &flag, NULL);	

	// 3. When a solution is found, using the tree structured broadcast to tell the other processes to shut down
		if (flag) {
            *num_inversions = nonce - nonce_start;
			/* Receive the message */
			MPI_Recv(&nonce, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			return;
		}
    }

    /* If we reach this point, then we did not find a solution. We will still
     * update the number of inversions before returning. */
    *num_inversions = nonce - nonce_start;
    return;
}

int main(int argc, char **argv) {
	MPI_Init(&argc, &argv);

    double time1 = MPI_Wtime();

    double total = 0;

    /* Total number of processes in this MPI communicator */
    int comm_sz;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    /* Get the rank of this process */
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* Hostname */
    char hostname[MPI_MAX_PROCESSOR_NAME];
    int name_sz;
    MPI_Get_processor_name(hostname, &name_sz);

    if (argc < 3) {
        printf("Usage: %s <difficulty> 'block data (string)'\n", argv[0]);
        return 0;
    }

    // TODO we have hard coded the difficulty to 20 bits (0x0000FFF). This is a
    // fairly quick computation -- something like 28 will take much longer.  You
    // should allow the user to specify anywhere between 1 and 32 bits of zeros.

// 1. Implement variable difficulty (the user can specify how many zeros they want to find)
    int input = atoi(argv[1]);	

    uint32_t difficulty = 0xFFFFFFFF;
    difficulty = difficulty >> input;
    if(rank == 0){
		printf("Difficulty Mask: ");
		print_binary32(difficulty);
		printf("\n");
    }


    // TODO the start and end values here work great for a single process.
    // However, we should split the entire range of nonces (from 0 to
    // UINT64_MAX) up across all available processes.
    char *block = argv[2];
    uint64_t start = 0;
    uint64_t end = UINT64_MAX;
    uint64_t nonce = 0;
    uint64_t hashes = 0;

    uint64_t comp_size = end/comm_sz;

// 2. Splitting the nonces up across multiple processes
    if(rank < comm_sz){ 
	    start = rank * comp_size;
	    end = (rank +1) * comp_size;
	    mine(block, difficulty, start, end, &nonce, &hashes);
    }
   
    if(nonce != 0){
    	int i = 0;	
		for(; i < comm_sz; ++i){
	    	if(i != rank){
				MPI_Send(&nonce, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    		}
    	}
    }
    // TODO if a solution is found, then we should let the other processes know
    // so they stop working. We should also retrieve the total number of hashes
    // computed by all the processes in the system and report the statistics
    // below.

// 4. Print out some timing info / statistics
	if (nonce != 0) {
        printf("Number of processes: %d\n", comm_sz);
        printf("Solution found by rank %d (%s):\n", rank, hostname);
        printf("Nonce: %llu\n", nonce);
        printf("Hash: ");
        print_hash();
    }

	MPI_Reduce(&hashes, &total, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    
    double time2 = MPI_Wtime();
    double overall_time = time2 - time1;
    double v = hashes / overall_time;

    if(rank == 0){
		printf("%llu hashes in %.2lfs (%.2lf hashes/sec)\n", hashes, overall_time, v);
    }

    MPI_Finalize();
    return 0;
}
