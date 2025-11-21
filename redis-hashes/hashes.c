#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h> // CHANGE
// manually call malloc a bunch of times
// call each hash function and see if tracked

// Compile with this command: gcc hashes.c -o hashes -lm

uint64_t const _sampling_interval = 1024;
double const sampling_rate = 1.0 / (double)(_sampling_interval);
uint32_t rand_seed = 12345;
int64_t remaining_bytes = -1 * _sampling_interval;

int sum_cpp = 0;
int sum_djb2 = 0;
int sum_sdbm = 0;
int sum_xor = 0;
int sum_pois = 0;

size_t djb2_hash(size_t ptr) {
    size_t hash = 5381;
    for (long unsigned int i = 0; i < sizeof(size_t); i++) {
        hash = ((hash << 5) + hash) + ((ptr >> (i * 8)) & 0xFF);
    }
    return hash;
}

size_t sdbm_hash(size_t ptr) {
    size_t hash = 0;
    for (long unsigned int i = 0; i < sizeof(size_t); i++) {
        hash = ((ptr >> (i * 8)) & 0xFF) + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

size_t xor_hash(size_t ptr) {
    // Convert pointer to integer type safely

    size_t hash = ptr;

    hash ^= hash >> 12;         // mix high bits down
    hash ^= hash << 25;         // mix low bits up
    hash ^= hash >> 27;         // final diffusion

    return hash;
}

uint64_t next_sample_interval(void) {
  /* exponential sampling*/
  double u = ((double)rand_r(&rand_seed) + 1.0 ) / ((double)RAND_MAX + 1.0);
  double value = -log(u) / sampling_rate;

  const int kMaxSamplingMultiplier = 20;
  const double min_value = 8.0;
  double max_value = _sampling_interval * kMaxSamplingMultiplier;
  if (value < min_value) value = min_value;
  if (value > max_value) value = max_value;
  return (size_t)value;
}

/* Allocate memory or panic */
void *wrap_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) return NULL;
    /* CHANGE START*/
    else {
        int cpp_hash_track = ((size_t)ptr & 0xFF) == 0; //C++ hash which just casts
        int djb2_hash_track = (djb2_hash((size_t)ptr) & 0xFF) == 0; //djb2 hash
        int sdbm_hash_track = (sdbm_hash((size_t)ptr) & 0xFF) == 0; //sdbm hash
        int xor_hash_track = (xor_hash((size_t)ptr) & 0xFF) == 0; //xor hash

        sum_cpp += cpp_hash_track;
        sum_djb2 += djb2_hash_track;
        sum_sdbm += sdbm_hash_track;
        sum_xor += xor_hash_track;


        int poisson_track = 0;
        uint64_t poisson_size = 0;

        remaining_bytes += size;
        if (remaining_bytes >= 0) {
            poisson_track = 1;
            uint64_t sampling_interval = _sampling_interval;
            size_t nsamples = remaining_bytes / sampling_interval;
            do {
                int samp_int = next_sample_interval();
                //printf("%d", samp_int);
                remaining_bytes -= samp_int;
                ++nsamples;
            } while (remaining_bytes >= 0);

            poisson_size = nsamples * sampling_interval;
        }
        sum_pois += poisson_track;

        // Columns: Call, Address, Size, HashTrack, PoissonTrack, PoissonSize
        //printf("malloc,%p,%zu,%d,%d,%d,%d,%d,%zu\n", ptr, size, cpp_hash_track, djb2_hash_track, sdbm_hash_track, xor_hash_track, poisson_track, poisson_size);
    }
    /* CHANGE END*/
    return ptr;
}


int main(void)
{
    for (int i = 0; i < 10000; ++i) {
        size_t size = (rand() % 2048) + 1; // Allocate between 1 and 2048 bytes
        void *ptr = wrap_malloc(size);
        if (!ptr) {
            fprintf(stderr, "Memory allocation failed\n");
            return 1;
        }
        //free(ptr);
    }
    printf("C++ Hash tracked: %d\n", sum_cpp);
    printf("DJB2 Hash tracked: %d\n", sum_djb2);
    printf("SDBM Hash tracked: %d\n", sum_sdbm);
    printf("XOR Hash tracked: %d\n", sum_xor);
    printf("Poisson tracked: %d\n", sum_pois);
    return 0;
}
