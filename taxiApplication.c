/*
  *
  * github.com/emrebicer
  *   Ömer Emre Biçer
  *     2016510014
  *        DEU
  *
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define TAXI_NUMBER 10
#define STUDENT_NUMBER 100
#define TAXI_CAPACITY_FOR_STUDENTS 4

//semaphore variables to avoid race-condition
sem_t taxis[TAXI_NUMBER];
sem_t remaining_student_sem;
//Global variables for taxis to reach them from anywhere.
int taxi_states[TAXI_NUMBER];
int taxi_numberOfStudents[TAXI_NUMBER];
int students_that_wait_for_transportation = STUDENT_NUMBER;
int student_ids_in_taxis [TAXI_NUMBER][TAXI_CAPACITY_FOR_STUDENTS];


void *student(void *id){
  //Identifying the student
  int studentNumber = (int)id;
  //Come to the school at a random time.
  sleep(rand() %10);

  //Initially a student is not in the taxi.(state 2 = waiting for a taxi.)
  int studentState = 2;
  //States
  //State 1 - Waiting in taxi.
  //A.K.A. Get into the taxi.
  //If taxi.numberOfStudents == 0 then wake up the driver.

  //State 2 - Waiting at the taxi stop.
  //If there is no taxi at the stop, students wait.

  while(1){
    if(studentState == 1){
      //Wait in taxi
      printf("Student #%d is waiting in the taxi.\n",studentNumber);
      sleep(1);
      //break the while loop and end this thread because the student tranportation is completed.
      break;
    }
    else if(studentState == 2){
      //Check if there is an empty taxi.
      printf("Student #%d is looking for an empty taxi.\n",studentNumber);
      /*
       *
       * Important note: better use a mutex lock to avoid read-write
       *   to global data from other threads at the same time.
       *    (Avoid race condition.)
       *
      */
      //Check if there are taxis that are waiting for more students.
      int i = 0;
      int student_has_found_a_taxi = 0;
      for(i = 0;i<TAXI_NUMBER;i++){

        //Lock the semaphore
        sem_wait(taxis+i);

        if(taxi_states[i] == 1){
          //Uncomment for debug.
          //printf("***** Found #%d taxi at state 1 ! *****\n", i);

          //This taxi is waiting for more students.
          //Get in to the taxi.
          printf("Student #%d get into the taxi #%d\n",studentNumber,i );
          taxi_numberOfStudents[i]++;

          //Add the student id to the taxi seat.
          int index;
          for(index = 0;index<TAXI_CAPACITY_FOR_STUDENTS;index++){
            if(student_ids_in_taxis[i][index] == -1){
              //If i. taxis index. seat is empty...
              student_ids_in_taxis[i][index] = studentNumber;
              break;
            }
          }

          //If all of the seats are full set the state of the taxi to 3(transport state.)
          if(taxi_numberOfStudents[i] >= TAXI_CAPACITY_FOR_STUDENTS){
            taxi_states[i] = 3;
          }

          //Debug --- this will only print when somehow semaphores won't work on recent OS.
          if(taxi_numberOfStudents[i] >= TAXI_CAPACITY_FOR_STUDENTS+1){
            printf("Taxi capacity violated.( ERR : Probably related to semaphore problem due recent OS) :%d\n",taxi_numberOfStudents[i]);
          }

          student_has_found_a_taxi = 1;
          studentState = 1;
          //Unlock the semaphore because this taxi is avaible at the taxi-station now.
          sem_post(taxis+i);
          break;
        }

        //Unlock the semaphore because this taxi is avaible at the taxi-station now.
        sem_post(taxis+i);
      }

      //Check if there are taxis with a driver sleeping.
      if(student_has_found_a_taxi == 0){
        //No taxi was found with students in it maybe there
        //is a taxi with a driver sleeping in it.
        for(i = 0;i<TAXI_NUMBER;i++){

          //Lock the semaphore
          sem_wait(taxis+i);

          if(taxi_states[i] == 2){
            //Uncomment for debug.
            //printf("***** Found #%d taxi at state 2 ! *****\n", i);


            //This taxi driver is sleeping wake the driver up.
            printf("Student #%d woke up the driver of taxi #%d\n",studentNumber,i );
            taxi_states[i] = 1;
            //Get in to the taxi.
            printf("Student #%d get into the taxi #%d\n",studentNumber,i );
            taxi_numberOfStudents[i]++;

            //Add the student id to the taxi seat.
            int index;
            for(index = 0;index<TAXI_CAPACITY_FOR_STUDENTS;index++){
              if(student_ids_in_taxis[i][index] == -1){
                //If i. taxis index. seat is empty...
                student_ids_in_taxis[i][index] = studentNumber;
                break;
              }
            }

            if(taxi_numberOfStudents[i] >= TAXI_CAPACITY_FOR_STUDENTS){
              taxi_states[i] = 3;
            }

            //Debug --- this will only print when somehow semaphores won't work on recent OS.
            if(taxi_numberOfStudents[i] >= TAXI_CAPACITY_FOR_STUDENTS + 1){
              printf("Taxi capacity violated.( ERR : Probably related to semaphore problem due recent OS) :%d\n",taxi_numberOfStudents[i]);
            }

            student_has_found_a_taxi = 1;
            studentState = 1;
            //Unlock the semaphore because this taxi is avaible at the taxi-station now.
            sem_post(taxis+i);
            break;
          }

          //Unlock the semaphore because this taxi is avaible at the taxi-station now.
          sem_post(taxis+i);
        }
      }
      if(student_has_found_a_taxi == 0){
        //Wait
        sleep(rand()%5+2);
      }
    }
  }
  return 0;
}

void *taxi(void *id){
  //Identifying the taxi.
  int taxiNumber = (int)id;
  //Each taxi has extra 4 seats for students
  int capacity = TAXI_CAPACITY_FOR_STUDENTS;
  int taxiState = 2; // sleeping state.
  int numberOfStudents = 0;//No students in the taxi initially


  //States
  //State 1 - collect student.
  //Also print (4 - numberOfStudent seats are empty!)

  //State 2 - idle.
  //If there is no student at the stop driver sleeps.

  //State 3 - full and transport
  //The taxi has 4 students in it it is full and it will transport


  while(1){
    //Lock the semaphore
    sem_wait(&remaining_student_sem);

    if(students_that_wait_for_transportation <= 0){
      //No more students are waiting in the stop.
      //End the taxi threads.

      //Unlock the semaphore before ending the thread
      sem_post(&remaining_student_sem);
      printf("No more students to transfer.Taxi #%d's driver will sleep.\n",taxiNumber );
      //Break the while loop and end the thread.
      break;
    }
    //Unlock the semaphore
    sem_post(&remaining_student_sem);

    //Lock the semaphore
    sem_wait(taxis+taxiNumber);
    taxiState = taxi_states[taxiNumber];
    numberOfStudents = taxi_numberOfStudents[taxiNumber];
    //Unlock the semaphore
    sem_post(taxis+taxiNumber);

    //Check the states and act for the recent state.
    if (taxiState == 2) {
      /* Taxi driver is sleeping */
      printf("Taxi #%d is sleeping.\n",taxiNumber );
    }
    else if(taxiState == 1){
      //Wait for more students

      /*
        If the seats are not full but
        if we know that no more students will come
        This taxi should start moving even though the capacity is not full.
      */

      //Get the remaining student num
      sem_wait(&remaining_student_sem);
      int rs = students_that_wait_for_transportation;
      int flag = 0;
      if(rs == numberOfStudents){
        flag = 1;
        //Well these last students are already in the taxi.
        //Even though the capacity is not full start the transportation.
        printf("***Even though the taxi is not full, it will start transporting with %d students, because no more students are waiting in the taxi stop.***\n",numberOfStudents);

        //Lock the semaphore
        sem_wait(taxis+taxiNumber);
        taxi_states[taxiNumber] = 3;
        //Unlock the semaphore
        sem_post(taxis+taxiNumber);
      }
      sem_post(&remaining_student_sem);

      if(flag == 0){
        int emptySeats = capacity-numberOfStudents;
        printf("Taxi #%d is waiting for %d more students.\n",taxiNumber,emptySeats);
      }

      //Check if the taxi is full.
      if(numberOfStudents == capacity){
        //Lock the semaphore
        sem_wait(taxis+taxiNumber);
        taxi_states[taxiNumber] = 3;
        //Unlock the semaphore
        sem_post(taxis+taxiNumber);
      }
    }
    else if(taxiState == 3){
      //Draw the current situation(students in the seats) of all the taxis.
      draw_taxis_with_students();

      int transportation_time = rand()%10+5;
      //Taxi is full start transporting the students
      printf("Taxi #%d is full and starting to transport.(%d sec)\n",taxiNumber,transportation_time );

      //Change the value of {transferred students} by locking the semaphore to avoid wrong calculations.

      //Lock semaphore
      sem_wait(&remaining_student_sem);
      students_that_wait_for_transportation = students_that_wait_for_transportation - numberOfStudents;

      //Unlock the semaphore
      sem_post(&remaining_student_sem);

      //Lock the semaphore
      sem_wait(taxis+taxiNumber);

      //print the taxi in transportation.
      printf("TAXI #%d [",taxiNumber);
      int index;
      for(index = 0;index<TAXI_CAPACITY_FOR_STUDENTS;index++){
        if(student_ids_in_taxis[taxiNumber][index] != -1){
          //This seat is not empty, print it.
          printf(" S#%d", student_ids_in_taxis[taxiNumber][index]);
        }
      }
      printf(" ] IS ON THE MOVE.\n" );


      //Unlock the semaphore
      sem_post(taxis+taxiNumber);

      //Transportation costs 5-15 seconds
      sleep(transportation_time);

      //Lock the semaphore
      sem_wait(taxis+taxiNumber);

      //Transportation completed mark all of the seats as empty.
      for(index = 0;index<TAXI_CAPACITY_FOR_STUDENTS;index++){
        student_ids_in_taxis[taxiNumber][index] = -1;
      }

      //Go back to the taxi-station and sleep until a student wakes you.
      taxi_states[taxiNumber] = 2;
      //Seats are empty now.
      taxi_numberOfStudents[taxiNumber] = 0;

      //Unlock the semaphore
      sem_post(taxis+taxiNumber);

    }
    //Avoid fast output.
    sleep(rand()%2+1);
  }

  return 0;
}

int draw_taxis_with_students(){
  /*
      This function draw all of the taxis with their
      seats marked with sitting student number.
  */

  int index1,index2;
  printf("----- Recent taxis -----\n");
  for(index1 = 0 ;index1<TAXI_NUMBER;index1++){
    printf("\n");
    //Lock the semaphore to avoid data conflict
    sem_wait(taxis+index1);
    printf("T #%d [",index1);
    for(index2 = 0;index2<TAXI_CAPACITY_FOR_STUDENTS;index2++){
      if(student_ids_in_taxis[index1][index2] != -1){
        //if {index1}. taxis {index2}. seat is not empty print it.
        printf(" S#%d",student_ids_in_taxis[index1][index2] );
      }
    }
    printf(" ]\n");
    //Unlock the semaphore
    sem_post(taxis+index1);
  }
  printf("\n------------------------\n");

  return 0;
}

int main(){
    int i,j;
    //set taxi states.
    for(i = 0 ;i<TAXI_NUMBER;i++){
      //All the drivers are sleeping(idle)
      //See taxi function for more information about states...
      taxi_states[i] = 2;
    }

    //set taxi_numberOfStudents to 0 initially
    for(i = 0 ;i<TAXI_NUMBER;i++){
      //All the taxis are empty at the beginning.
      taxi_numberOfStudents[i] = 0;
    }

    //initially all of the seats at taxis are -1
    //seath with an id of {-1} means that this seat is empty.
    for(i = 0;i<TAXI_NUMBER;i++){
      for(j = 0;j<TAXI_CAPACITY_FOR_STUDENTS;j++){
        student_ids_in_taxis[i][j] = -1;
      }
    }

    //Binary semaphore initialization for each taxi.
    for(i = 0;i<TAXI_NUMBER;i++){
      sem_init(taxis+i,0,1);
    }
    sem_init(&remaining_student_sem,0,1);

    int dummyReturn;

    int taxi_id = 0;
    //Thread creation for taxis
    pthread_t taxiThreads[TAXI_NUMBER];
    for(i=0;i<TAXI_NUMBER;i++) {  // thread creation for each taxi
       dummyReturn = pthread_create(taxiThreads+i, NULL, &taxi, (void*)taxi_id);
       taxi_id++;
    }

    int student_id = 0;
    //Thread creation for students
    pthread_t studentThreads[STUDENT_NUMBER];
    for(i=0;i<STUDENT_NUMBER;i++) {  // thread creation for each student
       dummyReturn = pthread_create(studentThreads+i, NULL, &student, (void*)student_id);
       student_id++;
    }

    //Join student threads
    for(i=0;i<STUDENT_NUMBER;i++) {  // thread join
      int err = pthread_join(studentThreads[i], NULL);
    }

    //Join taxi threads
    for(i=0;i<TAXI_NUMBER;i++) {  // thread join
      int err = pthread_join(taxiThreads[i], NULL);
    }


    printf("All of the students have taken a taxi!\n");
    return 0;
}
