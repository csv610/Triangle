#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#define REAL double
#define VOID void
#include "triangle.h"

void generate_points_in_circle(struct triangulateio *io, int n, double radius) {
    io->numberofpoints = n;
    io->pointlist = (REAL *) malloc(n * 2 * sizeof(REAL));
    for (int i = 0; i < n; i++) {
        double r = radius * sqrt((double)rand() / RAND_MAX);
        double theta = 2.0 * M_PI * ((double)rand() / RAND_MAX);
        io->pointlist[i * 2] = r * cos(theta);
        io->pointlist[i * 2 + 1] = r * sin(theta);
    }
    io->numberofpointattributes = 0;
    io->pointmarkerlist = NULL;
}

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <number_of_points>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    struct triangulateio in, out;
    double start, end;

    srand(time(NULL));

    generate_points_in_circle(&in, n, 1.0);
    in.numberofsegments = 0;
    in.numberofholes = 0;
    in.numberofregions = 0;

    out.pointlist = NULL;
    out.pointmarkerlist = NULL;
    out.trianglelist = NULL;
    out.segmentlist = NULL;
    out.segmentmarkerlist = NULL;

    /* 
       Switches:
       z: zero-based indexing
       c: encloses the convex hull with segments. 
          For a point set, these are the convex hull edges.
       Q: Quiet mode
       e: Edge list (optional, but let's use it if we want to see edges)
       N: No nodes output (saves memory if we only want segments)
    */
    start = get_time();
    triangulate("czQN", &in, &out, NULL);
    end = get_time();

    printf("Convex Hull Benchmark\n");
    printf("Points: %d\n", n);
    printf("Hull segments: %d\n", out.numberofsegments);
    printf("Time: %f seconds\n", end - start);

    free(in.pointlist);
    if (out.pointlist) trifree(out.pointlist);
    if (out.segmentlist) trifree(out.segmentlist);
    if (out.segmentmarkerlist) trifree(out.segmentmarkerlist);

    return 0;
}
