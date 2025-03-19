/*

  Name: Josue Trejo Ruiz
  ID:   1002232581

*/

// Copyright (c) 2020 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

/*** Constants that define parameters of the simulation ***/

#define MAX_SEATS 3        /* Number of seats in the professor's office */
#define professor_LIMIT 10 /* Number of students the professor can help before he needs a break */
#define MAX_STUDENTS 1000  /* Maximum number of students in the simulation */

#define CLASSA 0
#define CLASSB 1
#define CLASSC 2
#define CLASSD 3
#define CLASSE 4

/* TODO */
/* Add your synchronization variables here */

/* Basic information about simulation.  They are printed/checked at the end
 * and in assert statements during execution.
 *
 * You are responsible for maintaining the integrity of these variables in the
 * code that you develop.
 */

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static int students_in_office; /* Total number of students currently in the office */
static int classa_inoffice;    /* Total numbers of students from class A currently in the office */
static int classb_inoffice;    /* Total numbers of students from class B in the office */
static int students_since_break = 0;

static int current_class = -1;
static int consecutive = 0;
static int waitingA = 0;
static int waitingB = 0;
static int professor_on_break = 0;

typedef struct
{
  int arrival_time;  // time between the arrival of this student and the previous student
  int question_time; // time the student needs to spend with the professor
  int student_id;
  int class;
} student_info;

/* Called at beginning of simulation.
 * TODO: Create/initialize all synchronization
 * variables and other global variables that you add.
 */
static int initialize(student_info *si, char *filename)
{
  students_in_office = 0;
  classa_inoffice = 0;
  classb_inoffice = 0;
  students_since_break = 0;

  /* Initialize your synchronization variables (and
   * other variables you might use) here
   */

  current_class = -1;
  consecutive = 0;
  waitingA = 0;
  waitingB = 0;
  professor_on_break = 0;

  /* Read in the data file and initialize the student array */
  FILE *fp;
  if ((fp = fopen(filename, "r")) == NULL)
  {
    printf("Cannot open input file %s for reading.\n", filename);
    exit(1);
  }
  int i = 0;
  while ((fscanf(fp, "%d%d%d\n", &(si[i].class), &(si[i].arrival_time), &(si[i].question_time)) != EOF) &&
         i < MAX_STUDENTS)
  {
    i++;
  }
  fclose(fp);
  return i;
}

/* Code executed by professor to simulate taking a break.
 * You do not need to add anything here.
 */
static void take_break()
{
  printf("The professor is taking a break now.\n");
  sleep(5);
  assert(students_in_office == 0);
  students_since_break = 0;
}

/* Code for the professor thread. This is fully implemented except for synchronization
 * with the students.  See the comments within the function for details.
 */
void *professorthread(void *junk)
{
  printf("The professor arrived and is starting his office hours\n");

  /* Loop while waiting for students to arrive. */
  while (1)
  {
    pthread_mutex_lock(&mutex);
    // Wait until the professor helps 10 students and the office is empty.
    while (!(students_since_break >= professor_LIMIT && students_in_office == 0))
    {
      pthread_cond_wait(&cond, &mutex);
    }
    // Set the mutex so no new student enters
    professor_on_break = 1;
    pthread_mutex_unlock(&mutex);

    // Take break outside the critical region
    take_break();

    // REset variables
    pthread_mutex_lock(&mutex);
    students_since_break = 0;
    professor_on_break = 0;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
  }
  pthread_exit(NULL);
}

/* Code executed by a class A student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classa_enter()
{
  pthread_mutex_lock(&mutex);
  waitingA++;

  while (professor_on_break ||
         (students_in_office == MAX_SEATS) ||
         (current_class != -1 && current_class != CLASSA) ||
         (students_since_break >= professor_LIMIT) ||
         (current_class == CLASSA && consecutive >= 5 && waitingB > 0))
  {
    pthread_cond_wait(&cond, &mutex);
  }
  waitingA--;

  // If office is empty, set current class to CLASSA
  if (students_in_office == 0)
  {
    current_class = CLASSA;
    consecutive = 0;
  }
  students_in_office++;
  students_since_break++;
  consecutive++;
  classa_inoffice++;

  pthread_mutex_unlock(&mutex);
}

/* Code executed by a class B student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classb_enter()
{
  pthread_mutex_lock(&mutex);
  waitingB++;

  while (professor_on_break ||
         (students_in_office == MAX_SEATS) ||
         (current_class != -1 && current_class != CLASSB) ||
         (students_since_break >= professor_LIMIT) ||
         (current_class == CLASSB && consecutive >= 5 && waitingA > 0))
  {
    pthread_cond_wait(&cond, &mutex);
  }
  waitingB--;

  if (students_in_office == 0)
  {
    current_class = CLASSB;
    consecutive = 0;
  }
  students_in_office++;
  students_since_break++;
  consecutive++;
  classb_inoffice++;

  pthread_mutex_unlock(&mutex);
}

/* Code executed by a student to simulate the time he spends in the office asking questions.
 * You do not need to add anything here.
 */
static void ask_questions(int t)
{
  sleep(t);
}

/* Code executed by a class A student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classa_leave()
{
  pthread_mutex_lock(&mutex);
  students_in_office--;
  classa_inoffice--;

  if (students_in_office == 0)
  {
    // Reset the office to empty.
    current_class = -1;
    consecutive = 0;
    // If exactly professor_LIMIT students were helped, signal the professor.
    if (students_since_break >= professor_LIMIT)
    {
      pthread_cond_signal(&cond);
    }
  }
  // Wake up waiting threads.
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&mutex);
}

/* Code executed by a class B student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classb_leave()
{
  pthread_mutex_lock(&mutex);
  students_in_office--;
  classb_inoffice--;

  if (students_in_office == 0)
  {
    current_class = -1;
    consecutive = 0;
    if (students_since_break >= professor_LIMIT)
    {
      pthread_cond_signal(&cond);
    }
  }
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&mutex);
}

/* Main code for class A student threads.
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void *classa_student(void *si)
{
  student_info *s_info = (student_info *)si;

  /* enter office */
  classa_enter();

  printf("Student %d from class A enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classb_inoffice == 0);

  /* ask questions --- do not make changes to these three lines */
  printf("Student %d from class A starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class A finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classa_leave();

  printf("Student %d from class A leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main code for class B student threads.
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void *classb_student(void *si)
{
  student_info *s_info = (student_info *)si;

  /* enter office */
  classb_enter();

  printf("Student %d from class B enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classa_inoffice == 0);

  printf("Student %d from class B starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class B finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classb_leave();

  printf("Student %d from class B leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main function sets up simulation and prints report
 * at the end.
 * GUID: 355F4066-DA3E-4F74-9656-EF8097FBC985
 */
int main(int nargs, char **args)
{
  int i;
  int result;
  int num_students;
  void *status;
  pthread_t professor_tid;
  pthread_t student_tid[MAX_STUDENTS];
  student_info s_info[MAX_STUDENTS];

  if (nargs != 2)
  {
    printf("Usage: officehour <name of inputfile>\n");
    return EINVAL;
  }

  num_students = initialize(s_info, args[1]);
  if (num_students > MAX_STUDENTS || num_students <= 0)
  {
    printf("Error:  Bad number of student threads. Maybe there was a problem with your input file?\n");
    return 1;
  }

  printf("Starting officehour simulation with %d students ...\n", num_students);

  result = pthread_create(&professor_tid, NULL, professorthread, NULL);
  if (result)
  {
    printf("officehour:  pthread_create failed for professor: %s\n", strerror(result));
    exit(1);
  }

  for (i = 0; i < num_students; i++)
  {
    s_info[i].student_id = i;
    sleep(s_info[i].arrival_time);

    if (s_info[i].class == CLASSA)
    {
      result = pthread_create(&student_tid[i], NULL, classa_student, (void *)&s_info[i]);
    }
    else // student_type == CLASSB
    {
      result = pthread_create(&student_tid[i], NULL, classb_student, (void *)&s_info[i]);
    }

    if (result)
    {
      printf("officehour: thread_fork failed for student %d: %s\n", i, strerror(result));
      exit(1);
    }
  }

  /* Wait for all student threads to finish */
  for (i = 0; i < num_students; i++)
  {
    pthread_join(student_tid[i], &status);
  }

  /* tell the professor to finish. */
  pthread_cancel(professor_tid);

  printf("Office hour simulation done.\n");
  return 0;
}
