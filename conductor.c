#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define MAX_THREADS 2000000
pthread_t tid[MAX_THREADS];
pthread_t ctid;

sem_t studentASemaphore;
sem_t studentBSemaphore;

void *studentA(void *arg)
{
   sem_wait(&studentASemaphore);
   printf("Tick\n");
   return NULL;
}

void *studentB(void *arg)
{
   sem_wait(&studentBSemaphore);
   printf("Tock\n");
   return NULL;
}

void *conductor(void *arg)
{
   int i;
   for (i = 0; i < MAX_THREADS; i++)
   {
      if (i % 2 == 0)
      {
         sem_post(&studentASemaphore);
      }
      else
      {
         sem_post(&studentBSemaphore);
      }
      usleep(1000000);
   }
}

int main()
{
   int i;

   sem_init(&studentASemaphore, 0, 0);
   sem_init(&studentBSemaphore, 0, 0);

   pthread_create(&ctid, 0, conductor, NULL);

   for (i = 0; i < MAX_THREADS; i++)
   {
      if (i % 2 == 0)
      {
         int ret = pthread_create(&tid[i], 0, studentA, NULL);
         if (ret == -1)
            perror("Thread create failed: ");
      }
      else
      {
         int ret = pthread_create(&tid[i], 0, studentB, NULL);
         if (ret == -1)
            perror("Thread create failed: ");
      }
   }

   for (i = 0; i < MAX_THREADS; i++)
   {
      printf("Calling join on thread %d\n", i);
      pthread_join(tid[i], NULL);
   }

   // Here is where the threads do work
   //
   printf("Done with all the joins\n");

   return 0;
}
