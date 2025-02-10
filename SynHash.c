#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// State variables for Xorshift128X
static uint64_t s0 = 0;
static uint64_t s1 = 0;

/**
 * Xorshift128X PRNG/Hash Function
 * @param x - Input value to mix into the state.
 * @returns A 64-bit hash value.
 */
uint64_t Xorshift128X(uint64_t x) {
    uint64_t c = x << 30;
    uint64_t b = s0;
    uint64_t a = s1;

    // Output is the sum of the two state variables
    uint64_t result = (a + b);

    // Update state variables
    s0 = (a ^ c);
    b ^= (b << 23);
    s1 = (b ^ a ^ (b >> 18) ^ (a >> 5));

    return result;
}

/**
 * Hash a single block of 8 bytes (or fewer, padded with null bytes).
 * @param block - A block of up to 8 bytes.
 * @param length - Length of the block.
 * @returns A 64-bit hash value for the block.
 */
uint64_t hashBlock(const uint8_t *block, size_t length) {
    uint64_t result = 0;

    // Process each byte in the block
    for (size_t i = 0; i < length; i++) {
        result ^= Xorshift128X(block[i]);
    }

    // Reverse calculation for mixing
    for (size_t i = length; i > 0; i--) {
        result ^= Xorshift128X(block[i - 1]);
    }

    return result;
}

/**
 * Hash an entire message using a 256-bit hash function.
 * @param message - The input message to hash.
 * @param length - Length of the message.
 * @returns A 256-bit hash value as a hexadecimal string.
 */
void hash(const char *message, size_t length, char *output) {
    // Each hash value has to start from the same seeds
    s0 = 0x5555555555555555;
    s1 = 0xaaaaaaaaaaaaaaaa;

    const size_t subBlockSize = 8; // Each sub-block is 8 bytes (64 bits)
    const size_t blockSize = subBlockSize * 4; // Block size for total hash value (256 bits)
    size_t numBlocks = (length + blockSize - 1) / blockSize;
    numBlocks = numBlocks > 0 ? numBlocks : 1; // Handle empty string case

    // Initialize the final 256-bit hash as four 64-bit components
    uint64_t finalHash[4] = {0, 0, 0, 0};

    // Process each block
    for (size_t i = 0; i < numBlocks; i++) {
        // Extract the current block and pad it if necessary
        size_t blockStart = i * blockSize;
        size_t blockEnd = blockStart + blockSize;
        size_t blockLength = (blockEnd > length) ? (length - blockStart) : blockSize;

        // Declare the padded block array
        uint8_t paddedBlock[blockSize];
        // Initialize the padded block to zero
        memset(paddedBlock, 0, blockSize);
        // Copy the actual data into the padded block
        memcpy(paddedBlock, message + blockStart, blockLength);

        // Hash each sub-block
        uint64_t blockHash0 = hashBlock(paddedBlock, subBlockSize);
        uint64_t blockHash1 = hashBlock(paddedBlock + subBlockSize, subBlockSize);
        uint64_t blockHash2 = hashBlock(paddedBlock + 2 * subBlockSize, subBlockSize);
        uint64_t blockHash3 = hashBlock(paddedBlock + 3 * subBlockSize, subBlockSize);

        // Reverse blocks for mixing
        uint64_t blockHash4 = hashBlock(paddedBlock + 3 * subBlockSize, subBlockSize);
        uint64_t blockHash5 = hashBlock(paddedBlock + 2 * subBlockSize, subBlockSize);
        uint64_t blockHash6 = hashBlock(paddedBlock + subBlockSize, subBlockSize);
        uint64_t blockHash7 = hashBlock(paddedBlock, subBlockSize);

        // Combine the 64-bit block hashes into the 256-bit hash
        finalHash[0] ^= blockHash0 ^ blockHash4;
        finalHash[1] ^= blockHash1 ^ blockHash5;
        finalHash[2] ^= blockHash2 ^ blockHash6;
        finalHash[3] ^= blockHash3 ^ blockHash7;
    }

    // Convert the final hash to a hexadecimal string
    snprintf(output, 65, "%016lx%016lx%016lx%016lx",
             finalHash[3], finalHash[2], finalHash[1], finalHash[0]);
}

/**
 * Compute the Hamming distance between two 256-bit hashes.
 * @param hex1 - First hash as a hexadecimal string.
 * @param hex2 - Second hash as a hexadecimal string.
 * @returns The Hamming distance.
 */
int hammingDistance(const char *hex1, const char *hex2) {
    uint64_t bin1[4], bin2[4];
    sscanf(hex1, "%16lx%16lx%16lx%16lx", &bin1[0], &bin1[1], &bin1[2], &bin1[3]);
    sscanf(hex2, "%16lx%16lx%16lx%16lx", &bin2[0], &bin2[1], &bin2[2], &bin2[3]);

    int distance = 0;
    for (int i = 0; i < 4; i++) {
        uint64_t xorVal = bin1[i] ^ bin2[i];
        while (xorVal) {
            distance += xorVal & 1;
            xorVal >>= 1;
        }
    }
    return distance;
}

// Example Usage
int main() {
    const char *message = "hello, world 1";
    char hashedValue[65];

    hash(message, strlen(message), hashedValue);
    printf("256-bit Hash: %s\n", hashedValue);

    char hashedValue2[65];
    const char *message2 = "hello, world 2";
    
    hash(message2, strlen(message2), hashedValue2);
    printf("256-bit Hash: %s\n", hashedValue2);

    int distance = hammingDistance(hashedValue, hashedValue2);
    printf("Hamming Distance: %d\n", distance);

    return 0;
}