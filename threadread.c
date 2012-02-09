#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 10
#define FILE_PATH "/proc/lfprng"
#define BUF_SIZE 128

int read;
pthread_cond_t cond_read;
pthread_mutex_t mutex_read;

void *thread_func(void *ptr)
{
	FILE *fp;
	long id = *((long *) ptr);
	int i;
	char buf[BUF_SIZE];
	
	pthread_mutex_lock(&mutex_read);
	pthread_cond_wait(&cond_read, &mutex_read);
	
	if (!(fp = fopen(FILE_PATH, "r")))
	{
		fprintf(stderr, "thread %d: can't open file\n", id);
		pthread_exit(NULL);
	}
	i = fread(buf, sizeof(char), BUF_SIZE-1, fp);
	fclose(fp);
	
	buf[i] = '\0';
	printf("thread %d: %s\n", id, &buf);
	
	pthread_mutex_unlock(&mutex_read);
}

int main(int argc, char *argv[])
{
	int i, id[NUM_THREADS];
	pthread_t thread[NUM_THREADS];
	pthread_attr_t attr;
	FILE *fp;
	char buf[BUF_SIZE];
	
	pthread_mutex_init(&mutex_read, NULL);
	pthread_cond_init(&cond_read, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
	for (i = 0; i < NUM_THREADS; i++)
	{
		id[i] = i;
		pthread_create(&thread[i], NULL, thread_func, (void *) &id[i]);
	}

	if (!(fp = fopen(FILE_PATH, "w")))
	{
		fprintf(stderr, "parent: can't open file\n");
		return 1;
	}
	printf("parent: writing file\n");
	fprintf(fp, "42");
	fclose(fp);
	
	if (!(fp = fopen(FILE_PATH, "r")))
	{
		fprintf(stderr, "parent: can't open file\n");
		return 1;
	}
	i = fread(buf, sizeof(char), BUF_SIZE-1, fp);
	fclose(fp);
		
	buf[i] = '\0';
	printf("parent: %s\n", &buf);
	
	pthread_cond_broadcast(&cond_read);
	
	for (i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(thread[i], NULL);
	}
	
	pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&mutex_read);
	pthread_cond_destroy(&cond_read);
	pthread_exit(NULL);	
	return 0;
}
