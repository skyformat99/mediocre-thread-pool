#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define NUM_OF_RECORDS 10

int main(int argc, char const *argv[]) {
    assert(argc == 2 && "Input file not found.");

    double sum = 0.0;
    double requests_per_second = 0.0;

    FILE *in = fopen(argv[1], "r");
    assert(in != NULL && "Failed to open input file.");

    int records = NUM_OF_RECORDS;
    while (records--) {
        assert(EOF != fscanf(in, "%lf", &requests_per_second) &&
                "Insufficient record.");
        sum = sum + requests_per_second;
    }
    fclose(in);

    FILE *out = fopen("result/output.txt", "a");
    assert(out != NULL && "Failed to open output file.");
    fprintf(out, " %.2lf", sum / (1000 * NUM_OF_RECORDS));
    fclose(out);

    return 0;
}
