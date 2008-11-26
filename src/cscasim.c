#include <stdlib.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>

void print_matrix(double** d, int n, int m)
{
	int i,j;

	for (i=0; i < n; i++) {
		for (j=0; j < m; j++) {
			printf("%.1f ", d[i][j]);
		}
		printf("\n");
	}
}

/* NOTE: Is inlining equivalent to making this a macro?  Check out
 * always_inline (non-standard GCC feature).  According to the GCC manual it's
 * almost as fast as a macro (whatever that means). */
inline double fmin3(double a, double b, double c)
{
	double t;
	t = (a <= b || isnan (b)) ? a : b;
	t = (t <= c || isnan (c)) ? t : c;
	return t;
}

double** create_matrix(int* ns, double* sd,
					   int* nt, double* td)
{

	double** d;
	double accumulator;
	int i, j;
	int m, n;
	n = *ns;
	m = *nt;

	d = malloc(sizeof(double*) * (n + 1));
	for (i=0; i <= n; i++)
		d[i] = malloc(sizeof(double) * (m + 1));

	/* initialize matrix */

	d[0][0] = 0.0;
	accumulator = 0.0;
	for (i=0; i < n; i++) {
		accumulator += sd[i];
		d[i+1][0] = accumulator;
	}
	accumulator = 0.0;
	for (j=0; j < m; j++) {
		accumulator += td[j];
		d[0][j+1] = accumulator;
	}

	return d;
}

void free_matrix(double** d, int* ns) {

	int i;

	for (i=0; i <= *ns; i++)
		free(d[i]);
	free(d);

}

/* 
 * Computes the similarity of two scanpaths.  Fixation sites are defined using
 * x and y coordinates.  ns is the number of fixations, sx is an array with the
 * x coordinates of the fixations (length *ns), sy are the y-coordinates, sd
 * the fixation durations (usually in milliseconds)
 * */
void cscasim(int* ns, double* slon, double* slat, double* sd,
			 int* nt, double* tlon, double* tlat, double* td,
			 double* modulator,
			 double* result)
{
	double** d;
	double distance;
    double d_lon, cos_d_lon, sin_d_lon, cos_slat, cos_tlat, sin_slat, sin_tlat;
	double cost;
	int i, j;
	int m, n;
	n = *ns;
	m = *nt;

	/* allocate memory for 2-d matrix */
	d = create_matrix(ns, sd, nt, td);

	/* calculate distance */

	for (i=0; i < n; i++) {
		for (j=0; j < m; j++) {

            /* great circle distance using the Vincenty formula for better
             * precision */
			d_lon = slon[i] - tlon[j];
            cos_d_lon = cos(d_lon);
            sin_d_lon = sin(d_lon);
            cos_slat = cos(slat[i]);
            cos_tlat = cos(tlat[j]);
            sin_slat = sin(slat[i]);
            sin_tlat = sin(tlat[j]);

            distance =
                atan2(sqrt(pow(cos_tlat * sin(d_lon), 2)
                           + pow(cos_slat * sin_tlat
                              - sin_slat * cos_tlat * cos_d_lon, 2)),
                      sin_slat * sin_tlat + cos_slat * cos_tlat * cos_d_lon);
            distance *= 180/M_PI;

			/* cortical magnification */
			distance = pow(*modulator, distance);

			/* cost for substituting */
			cost = fabs((td[j] - sd[i]) * distance +
						(td[j] + sd[i]) * (1.0 - distance));

			/* selection of edit operation */
			d[i+1][j+1] = fmin3(d[i][j+1] + sd[i],
								d[i+1][j] + td[j],
								d[i][j]   + cost);
		}
	}

	*result = d[n][m];

	/* *result /= n + m; */

	/* free memory */

	free_matrix(d, ns);

}

/* 
 * Computes the similarity of two scanpaths.  Fixations sites are given as
 * regions of interest without spatial information about the arrangement of the
 * ROIs.  ns is the number of fixations, roi is an array with the id of the
 * fixated ROI (length *ns), sd the fixation durations (usually in
 * milliseconds)
 * */
void cscasim_roi(int* ns, int* sroi, double* sd,
				 int* nt, int* troi, double* td,
				 double* result)
{
	double** d;
	double distance;
	double cost;
	int i, j;
	int m, n;
	n = *ns;
	m = *nt;

	/* allocate memory for 2-d matrix */
	d = create_matrix(ns, sd, nt, td);

	/* calculate distance */

	for (i=0; i < n; i++) {
		for (j=0; j < m; j++) {
			/* roi distance */
			distance = sroi[i] == troi[j];

			/* cost for substituting */
			cost = fabs((td[j] - sd[i]) * distance +
						(td[j] + sd[i]) * (1.0 - distance));

			/* selection of edit operation */
			d[i+1][j+1] = fmin3(d[i][j+1] + sd[i],
								d[i+1][j] + td[j],
								d[i][j] + cost);
		}
	}

	*result = d[n][m];
	/* *result /= n + m; */

	/* free memory */

	free_matrix(d, ns);

}


/* vim:ts=4:sw=4
 * */