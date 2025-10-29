#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "reference_digits.h"

// Configuration
#define DIGIT_SIZE 784
#define NUM_REFS 100  
#define WORDS_PER_DIGIT 25 

// Memory-mapped I/O addresses
#define HW_REGS_BASE (0xff200000)
#define HW_REGS_SPAN (0x00200000)
#define HW_REGS_MASK (HW_REGS_SPAN - 1)
#define LED_PIO_BASE 0x0

// TODO: Function to count set bits (population count / hamming weight / number of 1's in the number)
uint32_t popcount(uint32_t x) {

}

// TODO: Calculate hamming distance between two digit vectors
uint32_t hamming_distance(const uint32_t *a, const uint32_t *b) {

}

// Read digit from file into uint32_t array
int read_digit_file(const char *filename, uint32_t *digit) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Error opening file");
        return -1;
    }

    // Initialize digit array to zero
    memset(digit, 0, WORDS_PER_DIGIT * sizeof(uint32_t));

    // Read 784 values (one per line)
    for (int i = 0; i < DIGIT_SIZE; i++) {
        int val;
        if (fscanf(fp, "%d", &val) != 1) {
            fprintf(stderr, "Error reading pixel %d\n", i);
            fclose(fp);
            return -1;
        }

        // Set bit if pixel is 1
        if (val != 0) {
            int word_idx = i / 32;
            int bit_idx = 31 - (i % 32);  
            digit[word_idx] |= (1U << bit_idx);
        }
    }

    fclose(fp);
    return 0;
}

// Control LEDs via memory-mapped I/O
int set_leds(uint32_t value) {
    volatile unsigned int *h2p_lw_led_addr = NULL;
    void *virtual_base;
    int fd;

    // Open /dev/mem
    if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
        printf("ERROR: could not open \"/dev/mem\"...\n");
        return -1;
    }

    // Get virtual address that maps to physical
    virtual_base = mmap(NULL, HW_REGS_SPAN, (PROT_READ | PROT_WRITE),
                       MAP_SHARED, fd, HW_REGS_BASE);

    if (virtual_base == MAP_FAILED) {
        printf("ERROR: mmap() failed...\n");
        close(fd);
        return -1;
    }

    // Get the address that maps to the LEDs
    h2p_lw_led_addr = (unsigned int *)(virtual_base + ((LED_PIO_BASE) & (HW_REGS_MASK)));

    // Write to LEDs (mask to 10 bits for LEDR[9:0])
    *h2p_lw_led_addr = value & 0x3FF;

    // Cleanup
    if (munmap(virtual_base, HW_REGS_SPAN) != 0) {
        printf("ERROR: munmap() failed...\n");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

// Main recognition function 
int recognize_digit(const char *filename) {
    uint32_t input_digit[WORDS_PER_DIGIT];
    uint32_t min_distance = UINT32_MAX;
    int best_match_index = -1;
    int predicted_digit = -1;

    // Read input digit from file
    printf("Reading digit from %s...\n", filename);
    if (read_digit_file(filename, input_digit) < 0) {
        return -1;
    }

    // Compute hamming distances for all references
    printf("Computing hamming distances...\n");
    for (int ref = 0; ref < NUM_REFS; ref++) {
        uint32_t dist = hamming_distance(input_digit, reference_digits[ref]);

        // TODO: Find the minimum - keep track of the minimum distance and corresponding label
        if (dist < min_distance) {

        }
    }

    // Display on LEDs
    if (set_leds(predicted_digit) < 0) {
        fprintf(stderr, "Warning: Could not set LEDs\n");
    }

    return predicted_digit;
}

int main(int argc, char *argv[]) {
    const char *filename;

    // Check arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <digit_file.txt>\n", argv[0]);
        fprintf(stderr, "Example: %s /media/fat_partition/digit.txt\n", argv[0]);
        return 1;
    }

    filename = argv[1];

    printf("MNIST Digit Recognition\n");
    printf("===================================\n");

    // Run recognition
    int result = recognize_digit(filename);

    if (result >= 0) {
        printf("\nSuccess! Check LEDs for digit guess.\n");
        return 0;
    } else {
        fprintf(stderr, "\nRecognition failed!\n");
        return 1;
    }
}
