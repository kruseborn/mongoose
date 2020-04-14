/* Performs the QR decomposition and solves the least squares problem for Ax=b
 for the polynomial fit through the data points supplied by user.
 The basis in the poynomial space of degree <= n is chosen to be the firnst n Chebyshev polynomials.
 Their coefficients are read from the file chebcoef.txt.
 The maximal possible degree of the fitting polynomial is 15.
 The data are read from the file mydata.txt.
 The data points are assumed to be uniformly distributed on the interval [0,1]. */


#include "houseHolder.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

#define PI 3.141592653589793
#define mabs(a) ((a) >= 0 ? (a) : -(a))
#define sgn(a) ((a) == 0 ? 0 : ((a) > 0 ? 1 : -1))
#define max(a, b) ((a) >= (b) ? (a) : (b))
#define min(a, b) ((a) <= (b) ? (a) : (b))

#define NCHEB 16       /* the matrix of Chebyshev coefficients read from chebcoef is NCHEB*NCHEB */
#define N 16           /* The degree of the fitting polynomial + 1,  N must be <= NCHEB */
#define NDATAMAX 10000 /* the maximal length of thedata file */
#define NOUT 100       /* the resulting polynomial is evaluated at NOUT points */

void readdata(void);
void fitdata(void); /* forms Ax=b and calles the least squares solver */
void lsqqr(float *a, float b[], float coef[], long ncol, long nrow);
/* solves the least squares problem using the Householder reflections */
void chebmatrix(float *a, float h, long ncol, long nrow);
/* forms the matrix A in Ax=b */

long ndata;                /* the number of the data points read from the data file */
float cheb[NCHEB][NCHEB]; /* array of chebyshev coefficients */
float *data;              /* pointer to the data */
float coef[N];            /* coefs of the fitting polynomial */
float p[NOUT];            /* values of the fitting polynomial
           FILE *fcheb, *fdata;
           
           /***************************/

/*-----------------*/

void chebmatrix(float *a, float h, long ncol, long nrow) {
  long i, m, j, nr1 = nrow - 1;
  float y, aux;

  for (i = 0; i < nrow; i++) {
    y = 2.0f * i * h - 1.0f;       /* rescale the data points to [-1,1] */
    for (j = 0; j < ncol; j++) { /* evaluate chebyshev polynomials at the data points */
      m = j;
      aux = cheb[j][j];
      for (m = j - 1; m >= 0; m--)
        aux = aux * y + cheb[j][m];
      *(a + i * ncol + j) = aux; /* a_{i,j} = aux */
    }
  }
}

/*---------------------------------------------------------*/
void lsqqr(float *a, float b[], float coef[], long ncol, long nrow) {
  long i, m, l;
  float normx, aaux, baux;
  std::vector<float> aux(ncol), u(nrow);
  std::vector<float> c(nrow); /* weight matrix */

  /* put larger weights at the ends interval to reduce the error at the ends of the interval */
  /* Ax = b   <==> CAx = Cb where C = diag{c[0],c[1],...,c[nrow]}  */
  for (m = 0; m < nrow; m++)
    c[m] = 1.0f;
  for (m = 0; m < min(nrow / 2, 10); m++)
    c[m] = float(10 - m);
  for (m = nrow - 1; m > max(nrow / 2, nrow - 9); m--)
    c[m] = float(9 - nrow + m);
  /* compute CA and Cb */
  for (m = 0; m < nrow; m++) {
    for (i = 0; i < ncol; i++) {
      (*(a + i + m * ncol)) *= c[m]; /* a_{m,i} = a_{m,i} * c_m */
    }
    b[m] *= c[m]; /* b_m = b_m * c_m */
  }

  /* Start the QR algorithm via Householder reflections */
  for (i = 0; i < ncol; i++) {
    /*form vector u=House(a(i:nrow,i))*/
    normx = 0.0;
    for (m = 0; m < nrow - i; m++) {
      aaux = *(a + (i + m) * ncol + i); /* aaux = a_{m+i,i} */
      normx += aaux * aaux;             /* normx = normx + aaux*aaux */
    }
    aaux = *(a + i * ncol + i); /* aaux = a_{i,i} */
    u[0] = float(aaux + sgn(aaux) * sqrt(normx));
    for (m = 1; m < nrow - i; m++)
      u[m] = *(a + (i + m) * ncol + i); /* u[m] = a_{i+m,i} */
    normx = 0.0;
    for (m = 0; m < nrow - i; m++)
      normx += u[m] * u[m]; /* normx = normx + u[m]*u[m] */
    /* compute (I-2uu^t)a(i:nrow,i:ncol) */
    for (l = 0; l < ncol - i; l++) {
      aux[l] = 0.0;
      for (m = 0; m < nrow - i; m++) {
        aux[l] += u[m] * (*(a + (i + m) * ncol + i + l)); /* aux[l] = aux[l] + u[m]*a_{i+m,i+l} */
      }
      aux[l] *= float(-2.0 / normx); /* aux[l] = -2*aux[l]/normx */
      for (m = 0; m < nrow - i; m++) {
        *(a + (i + m) * ncol + i + l) += u[m] * aux[l]; /* a_{i+m,i+l} = a_{i+m,i+l} + u[m]*aux[l] */
      }
    }
    /* compute (Q^T)*b */
    baux = 0.0;
    for (m = 0; m < nrow - i; m++)
      baux += u[m] * b[i + m]; /* baux = baux + u[m]*b[i+m] */

    baux *= float(-2.0 / normx);
    for (m = 0; m < nrow - i; m++)
      b[i + m] += baux * u[m]; /* b[i+m] = b[i+m] + baux*u[m] */
  }
  /* find the coefficients for the fitting polynomial p = sum_{j=0}^{ncol} coef[j]*P_j,
     where P_j is the j-th Chebyshev polynomial */
  for (i = ncol - 1; i >= 0; i--) {
    baux = 0.0;
    for (m = i + 1; m < ncol; m++) {
      baux += (*(a + i * ncol + m)) * coef[m]; /* baux = baux + a_{j,m}*coef[m] */
    }
    coef[i] = (b[i] - baux) / (*(a + i * ncol + i));
  }
}

/*--------------------------*/
void fitdata() {
  std::vector<float> b(ndata);
  float *a;
  const float h = 1.0f / (ndata - 1);   /* interval between the data points */
  const float hout = 1.0f / (NOUT - 1); /* interval between the output points */
  long j, k, m;
  float x, poly;

  a = (float *)malloc(ndata * N * sizeof(float)); /* allocate memory for the matrix A */
  chebmatrix(a, h, N, ndata);                       /* form the matrix A */
  for (j = 0; j < ndata; j++)
    b[j] = *(data + j);
  lsqqr(a, b.data(), coef, N, ndata);
  /* evaluate the polynomial at the uniformly distributed NOUT points */
  for (j = 0; j < NOUT; j++) {
    x = 2.0f * j * hout - 1.0f;
    p[j] = 0.0f;
    for (k = 0; k < N; k++) {
      poly = cheb[k][k];
      for (m = k - 1; m >= 0; m--)
        poly = poly * x + cheb[k][m]; /* evaluate Chebyshev polynomial  P_k */
      p[j] = p[j] + coef[k] * poly;   /* evaluation of the fitting polynomial */
    }
  }
}
