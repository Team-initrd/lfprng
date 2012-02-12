#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 100
#define FILE_PATH "/proc/lfprng"
#define BUF_SIZE 128

int total_threads;
int read;
unsigned long long ref[120];
unsigned long long results[120];
pthread_cond_t cond_read;
pthread_mutex_t mutex_read;

void *thread_func(void *ptr)
{
	FILE *fp;
	long id = *((long *) ptr);
	int i;
	char buf[BUF_SIZE];
	unsigned long long x;

	pthread_mutex_lock(&mutex_read);
	pthread_cond_wait(&cond_read, &mutex_read);
	
	/*if (!(fp = fopen(FILE_PATH, "r")))
	{
		fprintf(stderr, "thread %d: can't open file\n", id);
		pthread_exit(NULL);
	}*/
	//i = fread(buf, sizeof(char), BUF_SIZE-1, fp);
	
    for (i = id; i < 120; i+=total_threads) {
        fp = fopen(FILE_PATH, "r");
        fscanf(fp, "%lu", &x);
        results[i] = x;
#ifdef DEBUG
        printf("thread: %ld : %llu\n", i, x);
#endif
        fclose(fp);
    }
    
    //fclose(fp);
	
	//buf[i] = '\0';
	//printf("thread %d: %s\n", id, &buf);
	
	pthread_mutex_unlock(&mutex_read);
    pthread_exit(NULL);
    
}

int main(int argc, char *argv[])
{
	int i, id[NUM_THREADS];
	pthread_t thread[NUM_THREADS];
	pthread_attr_t attr;
	FILE *fp;
	char buf[BUF_SIZE];
	int n;
    unsigned long long seed = 12345;
    int tests[6]={2,3,4,5,6,10};
    int m;
    int num_threads;
    int error = 0;

// seed
    fp = fopen(FILE_PATH, "w");
    fprintf(fp, "12345 1", seed);
    fclose(fp);

    printf("CALCULATING REFERENCE VALUES\n");
    //fp = fopen(FILE_PATH, "r");
    for (n = 0; n < 120; n++) {
        fp = fopen(FILE_PATH, "r");
        fscanf(fp, "%llu", &(ref[n]));
        
        //fscanf(fp, "%s", buf);
#ifdef DEBUG
        printf("ref %d: %llu\n", n, ref[n]);
#endif
        fclose(fp);
    }
    //fclose(fp);
    for (m = 0; m < 6; m++) {
        error = 0;
	pthread_mutex_init(&mutex_read, NULL);
	pthread_cond_init(&cond_read, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
    //for (m = 0; m < 6; m++) {
        total_threads = tests[m];
        printf("TESTING WITH NUM THREADS: %d\n", total_threads);
    //total_threads = NUM_THREADS;

	for (i = 0; i < total_threads; i++)
	{
		id[i] = i;
		pthread_create(&thread[i], NULL, thread_func, (void *) &id[i]);
	}


    fp = fopen(FILE_PATH, "w");
    fprintf(fp, "12345 0", seed);
    fclose(fp);

	/*if (!(fp = fopen(FILE_PATH, "w")))
	{
		fprintf(stderr, "parent: can't open file\n");
		return 1;
	}
	printf("parent: writing file\n");
	fprintf(fp, "12345 0");
	fclose(fp);
	*/

    /*
	if (!(fp = fopen(FILE_PATH, "r")))
	{
		fprintf(stderr, "parent: can't open file\n");
		return 1;
	}
	i = fread(buf, sizeof(char), BUF_SIZE-1, fp);
	fclose(fp);
		
	buf[i] = '\0';
	printf("parent: %s\n", &buf);
	*/




	pthread_cond_broadcast(&cond_read);
	
	for (i = 0; i < total_threads; i++)
	{
		pthread_join(thread[i], NULL);
	}
	
	/*pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&mutex_read);
	pthread_cond_destroy(&cond_read);
	pthread_exit(NULL);	
*/

    printf("TESTING VALUES FOR THREADS: %d\n", total_threads);
    for (i = 0; i < 120; ++i) {
        if (ref[i] != results[i])
            error = i;
#ifdef DEBUG
        printf("n: %d ref: %llu result: %llu\n", i, ref[i], results[i]);
#endif
    }

    if (error)
        printf("There was an error with thread_num: %d, error index: %d\n", total_threads, error);
    else
        printf("NO ERROR with num threads: %d\n", total_threads);
    printf("=====================\n");

    
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&mutex_read);
    pthread_cond_destroy(&cond_read);
    }
    pthread_exit(NULL);
	return 0;
}
