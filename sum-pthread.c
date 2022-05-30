#include <stdio.h>
#include <stdlib.h>  // rand
#include <sys/time.h>
#include <pthread.h>
#include <sys/param.h>  // MIN

// usage
// clang -pthread -Ofast bar.c -o bar && time ./bar
//
// -pthread doesn't seem to matter.
// -Ofast gives strange results:
//   - if you omit it, you get the expected behavior
//   - if you include it, both non-threaded and batched ones
//     become significantly faster than the strided.
//   - if you select nthr=6 (from 6 performance cores on macbook)
//     then Ofast gives more reasonable results.
// -O1 has non-threaded slowest, and batched like 20% faster
// - when you increase nthr you really see the difference between
//   stride and batch since the former has cache misses.

typedef struct timeval tv;

float *randarr(long sz) {
    float *a = malloc(sz * sizeof(float));
    long i;
    for (i = 0; i < sz; ++i) {
        a[i] = (float) rand() / RAND_MAX - 0.5;
    }
    return a;
}

tv now() {
    tv x;
    gettimeofday(&x, NULL);
    return x;
}

void measure(tv end, tv beg, char *msg) {
    tv diff;
    timersub(&end, &beg, &diff);
    printf("%s in %ld.%06d sec\n", msg, diff.tv_sec, diff.tv_usec);
}

float compute_sum(float *arr, long beg, long stride, long end) {
    float s = 0;
    long i;
    for (i = beg; i < end; i += stride) {
        s += arr[i];
    }
    return s;
}

struct Param {
    float *arr, *res;
    long beg, stride, end;
};

typedef struct Param Pm;

Pm make_param(float *arr, float *res, long beg, long stride, long end) {
    Pm pm = {arr, res, beg, stride, end};
    return pm;
}

Pm stride_make_param(float *arr, float *res, long i, long nthr, long sz) {
    return make_param(arr, res + i, i, nthr, sz);
}

long ceildiv(long num, long den) {
    // https://stackoverflow.com/a/14878734/2990344
    return num / den + (num % den != 0);
}

Pm batch_make_param(float *arr, float *res, long i, long nthr, long sz) {
    long tdim = ceildiv(sz, nthr), end = MIN(sz, (i+1) * tdim);
    return make_param(arr, res + i, i * tdim, 1, end);
}

typedef Pm ParamFunc(float *, float *, long, long, long);

void *thunk_compute_sum(void *data) {
    Pm *pm = data;
    *pm->res = compute_sum(pm->arr, pm->beg, pm->stride, pm->end);
    return NULL;
}

float thread_compute_sum(float *arr, long sz, int nthr, ParamFunc make) {
    float *res = malloc(nthr * sizeof(float));
    Pm *ps = malloc(nthr * sizeof(Pm));
    pthread_t *ts = malloc(nthr * sizeof(pthread_t));
    int i;
    for (i = 0; i < nthr; ++i) {
        ps[i] = make(arr, res, i, nthr, sz);
        pthread_create(ts + i, NULL, thunk_compute_sum, ps + i);
    }
    for (i = 0; i < nthr; ++i) {
        pthread_join(ts[i], NULL);
    }
    free(ts);
    free(ps);
    float sum = compute_sum(res, 0, 1, nthr);
    free(res);
    return sum;
}

void measure_thread_sum(float *arr, long sz, int nthr, ParamFunc make, char *descr) {
    tv start = now();
    float sum = thread_compute_sum(arr, sz, nthr, make);
    measure(now(), start, descr);
    printf("%d threads, sz = %ld, %s sum = %f\n\n", nthr, sz, descr, sum);
}

int main() {
    long sz = 1L << 30;
    tv t0 = now();
    float *arr = randarr(sz);
    tv t1 = now();
    measure(t1, t0, "random array");
    printf("\n");
    float sum0 = compute_sum(arr, 0, 1, sz);
    tv t2 = now();
    measure(t2, t1, "no thread");
    printf("no thread sum is %f\n\n", sum0);
    measure_thread_sum(arr, sz, 1, stride_make_param, "stride");
    measure_thread_sum(arr, sz, 1, batch_make_param, "batched");
    measure_thread_sum(arr, sz, 6, stride_make_param, "stride");
    measure_thread_sum(arr, sz, 6, batch_make_param, "batched");
    measure_thread_sum(arr, sz, 8, stride_make_param, "stride");
    measure_thread_sum(arr, sz, 8, batch_make_param, "batched");
    measure_thread_sum(arr, sz, 24, stride_make_param, "stride");
    measure_thread_sum(arr, sz, 24, batch_make_param, "batched");
    free(arr);
}
