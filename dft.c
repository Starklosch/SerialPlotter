#include <complex.h>
#include <fftw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*
 * Compile with the following arguments:
 *      gcc fftw3_r2c_usage.c -o fftw3_r2c_usage -lfftw3 -lm
 *
 * Make sure that you installed fftw3 in your machine first.
 */

#define abs(X) (X < 0 ? -X : X)
#define magnitude(X) sqrt(X[0] * X[0] + X[1] * X[1])

int main(int argc, char* argv[])
{
    #define N 2000
    //double in[] = { 5, 4, 3, 2, 0, 0, 0, 0, 0 };	/* Example input */
    double in[N];	/* Example input */
    double pi = 3.14159265359;

    for (int i = 0; i < N; i++)
        in[i] = 3 * sin(2 * pi * i / (double)N) + sin(2 * pi * 4 * i / (double)N) + 0.5 * sin(2 * pi * 7 * i / (double)N);

    fftw_complex* out; /* Output */
    fftw_plan p; /* Plan */

    /*
     * Size of output is (N / 2 + 1) because the other remaining items are
     * redundant, in the sense that they are complex conjugate of already
     * computed ones.
     *
     * CASE SIZE 6 (even):
     * [real 0][complex 1][complex 2][real 3][conjugate of 2][conjugate of 1]
     *
     * CASE SIZE 5 (odd):
     * [real 0][complex 1][complex 2][conjugate of 2][conjugate of 1]
     *
     * In both cases the items following the first N/2+1 are redundant.
     */
    out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (N / 2 + 1));

    /*
     * An fftw plan cares only about the size of in and out,
     * not about actual values. Can (and should) be re-used.
     */
    p = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);

    /*
     * Execute the dft as indicated by the plan
     */
    fftw_execute(p);

    /*
     * Print the N/2+1 complex values computed by the DFT function.
     */
    int i;
    for (i = 0; i < N / 2 + 1; i++) {
        if (abs(out[i][0]) > 0.000000001 || abs(out[i][1]) > 0.000000001)
        printf("out[%d] = {%.20f, %.20fi} = %f\n", i, out[i][0], out[i][1], magnitude(out[i]));
    }

    /*
     * Clean routine
     */
    fftw_destroy_plan(p);
    fftw_free(out);

    return 1;
}