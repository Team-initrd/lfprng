#include <stdio.h>
#include <stdlib.h>
#include <omp.h>


/* The following is included as a reference user-space implementation */

//**********************************************************
// Parallel Pseudo random number generator:
//
// USAGE:
//
//  The pseudo random sequence is seeded with a range
//
//            void seed(lower_limit, higher_limit)
//   
//  and then subsequent calls to the random number generator 
//  generates values in the sequence:
//
//            double random()
//
//  A leap frog method is used to assure non-overlapping
//  sequences for each thread.
//
//  Note: these functions are to be called from inside the
//  the OpenMP parallel region that will use the sequence.
//
//  BACKGROUND:
//
//  We are using a modulus of 2^31-1 and a multiplier from 
//  the Hoaglin LCGs in the following article:
//
//    http://random.mat.sbg.ac.at/~charly/server/node3.html#lcg
//
//   we are using a zero addend just to make the leap frog 
//   algorithm easier to implement.
//
//  HISTORY:
//
//  9/2008: Written by Tim Mattson by cutting and pasting 
//  from a generator written by Larry Meadows
//
//***********************************************************


static unsigned long long MULTIPLIER  = 764261123;
static unsigned long long PMOD        = 2147483647;
static unsigned long long mult_n;
double random_low, random_hi;

#define MAX_THREADS 128
#define FILE_PATH "/proc/lfprng"

static unsigned long long pseed[MAX_THREADS][4]; //[4] to padd to cache line
                                                 //size to avoid false sharing
unsigned long long random_last = 0;
#pragma omp threadprivate(random_last)


double myrandom()
{
  unsigned long long random_next;
  double ret_val;

  // 
  // compute an integer random number from zero to mod
  //
  random_next = (unsigned long long)((mult_n  * random_last)% PMOD);
  random_last = random_next;

  //
  // shift into preset range
  //
  ret_val = ((double)random_next/(double)PMOD)*(random_hi-random_low)+random_low;
  return ret_val;
}

//
// set the seed, the multiplier and the range
//
void seed(unsigned long long iseed, double low_in, double hi_in)
{
  int i, id, nthreads;
  id = omp_get_thread_num();

   #pragma omp single
  {
    if(low_in < hi_in)
      { 
	random_low = low_in;
	random_hi  = hi_in;
      }
    else
      {
	random_low = hi_in;
	random_hi  = low_in;
      }
  
    //
    // The Leapfrog method ... adjust the multiplier so you stride through
    // the sequence by increments of "nthreads" and adust seeds so each 
    // thread starts with the right offset
    //

    nthreads = omp_get_num_threads();
    if ( iseed == 0 )
      iseed = PMOD/MULTIPLIER;     // just pick a reasonable seed
    pseed[0][0] = iseed;
    mult_n = MULTIPLIER;
    for (i = 1; i < nthreads; ++i)
      {
	iseed = (unsigned long long)((MULTIPLIER * iseed) % PMOD);
	pseed[i][0] = iseed;
	mult_n = (mult_n * MULTIPLIER) % PMOD;
      }

  }
  random_last = (unsigned long long) pseed[id][0];
}


double bound(unsigned long long num, double min, double max) {
    return ((double)num/(double)PMOD)*(max-min)+min;
}


/*  This ends the reference openMP implementation.  The following is the core testing loop. */

int tests[6]={2,3,4,5,6,10};

int main(int argc, char **argv) {
  unsigned long long baseref[120];
  unsigned long long testref[120];
  unsigned long long myseed = 123456789, setseed;
  int i, j;
  FILE *fp;
  unsigned long long myrand;
  
  setseed=myseed;
  //seed(setseed, 0.0, 1.1);
  fp = fopen(FILE_PATH, "w");
  fprintf(fp, "%d 1", setseed);
  //printf("%d 0.0 1.1 1\n", setseed);
  fclose(fp);
  
  for (i=0; i<120; i++) {
    fp = fopen(FILE_PATH, "r");
    fscanf(fp, "%llu", &myrand);
    fclose(fp);
    
    baseref[i] = myrand; //myrandom();
    printf("%llu\n", myrand);
  }

  for (j=0; j<6; j++) {
    int numthreads = tests[j];
    unsigned long long sum;

    omp_set_num_threads(numthreads);
    sum = 0;

    //printf("NUM THREADS: %d\n", omp_get_num_threads());


#pragma omp parallel
{
    printf("parallel time\n");
}
    setseed=myseed;
    //seed(setseed,0.0,1.0);
    fp = fopen(FILE_PATH, "w");
    fprintf(fp, "%d 1", setseed);
    fclose(fp);

#pragma omp parallel private(fp, myrand) reduction(+:sum)
    {
      printf("NUM THREADS: %d\n", omp_get_num_threads());
      setseed=myseed;
      //#pragma omp barrier
      //seed(setseed,0.0,1.0);
      //#pragma omp single
      if (0 && omp_get_thread_num() == numthreads - 2)
      {
        fp = fopen(FILE_PATH, "w");
        fprintf(fp, "%d 1", setseed);
        fclose(fp);
      }
      #pragma omp barrier
#pragma omp for ordered
      for (i=0; i<120; i++) {
          //unsigned long long myrand;
        fp = fopen(FILE_PATH, "r");
        fscanf(fp, "%llu", &myrand);
        fclose(fp);
    
    printf("i = %d = myrand: %llu ref: %llu\n", i, myrand, baseref[i]);
	sum += abs(myrand - baseref[i]);
	//sum += myrand - baseref[i];
      }
    }    
    printf(" Diff for %i threads is %llu\n", numthreads, sum);

  }
  printf("\n");
}
