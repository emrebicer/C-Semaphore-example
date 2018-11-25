# C-Semaphore-example

## Prerequisites
Don't forget that you should include –lpthread flag to compile and succesfully run this program.

### An example compile with gcc
```
gcc taxiApplication.c –o app –lpthread
```

and
```
./app
```
to run.

## Description
This program is an example of how semaphores should be used in a multithreaded c programs.
The idea is that there are students waiting at the enterance of the University campus at a taxi stop.The taxis are there to get the students to the library / classes.

There are 3 states for **taxi drivers**;
* **Collecting students.**
* **idle / sleeping.**
* **The taxi is full and will start transporting.**

Also the taxi-driver will wait until the capacity of the taxi is full.(Unless there are no more students waiting at the taxi-stop)

And there are 2 states for the **students**;
* **Waiting in the taxi.**
* **Waiting at the taxi stop.**

The taxis and students are created as a thread and they all execute at the same time.
But they do read and write some common data.
To prevent **race-condition** problems we have to use semaphore structure.

## Semaphore
```c
sem_wait(&our_semaphore);
```
This function locks the sem_t variable named our_semaphore.
By locking it, you make sure that only this thread is accessing the specific data.
```c
sem_post(&our_semaphore);
```
And with this function, you simply tell other threads that this thread is done with the common data, so they can now acces is.

E.g.
```C
sem_wait(&our_semaphore);     //Locks the semaphore
//Critical region

commonData++;                 //Handle common data here
sem_post(&our_semaphore);     //Unlocks the semaphore
```
