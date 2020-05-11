
#include "P6.h"

extern void sleep();
extern void sem_post();
extern void sem_wait();

int fork_and_knife[ 16 ] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};           // 16 forks and knives.  create semaphor lock, 1 for available and 0 for occupied.
int mutex_lock = 1;           // mutex-lock



void* main_P6(){
  for ( int i = 0; i < 16; i++ ){     // call fork 16-times to create 16 philosophers.
    if ( fork() == 0 ){

      int id = ( i<8 ) ? (i*2) : 2*(i-8)+1;              // give the philosophers unique ID. Philosophers who have even ID should eat first, then the odd-ID ones
      int fork = id;      // match the ID to the fork
      int knife;                                                      // the knife should be the one after the fork. The last
      if (id != 15 ){ knife = id + 1; } else { knife = 0; }           // the last philosopher should use the first knife.

      char id_char [2];                                                   // convert the ID to characters in order to print.
      itoa ( id_char, id);
        write( STDOUT_FILENO, "Philosopher is Thinking. ID = ", 31 );
        write( STDOUT_FILENO, id_char, 2 );
        write( STDOUT_FILENO, "\n", 1 );

      while(1) {
        sleep( id );                    // initially, philosophers should think, before they get the lock.
        sem_wait( &mutex_lock );        // forever check if they get a lock
        if ( fork_and_knife[ fork ] == 1 && fork_and_knife[ knife ] == 1 ){     // a philosopher can eat iff. he has a fork AND a knife.
          sem_wait( &fork_and_knife[ fork ] );                                  // get the lock for both fork and knife. start eating.
          sem_wait( &fork_and_knife[ knife ] );
          sem_post( &mutex_lock );                                              // release the mutex_lock so that other philosophers can
          write( STDOUT_FILENO, "Philosopher is Eating, ID = ",28);             // find available forks and knives while he is eating.
          write( STDOUT_FILENO, id_char ,2);
          write( STDOUT_FILENO, "\n", 1 );
        }
        else {                                                                  // if he can NOT have available knife or fork or both,
          sem_post( &mutex_lock );                                              // release the mutex lock and resume thinking.
          write( STDOUT_FILENO, "Philosopher has no Fork or Knife, ID = ",40);
          write( STDOUT_FILENO, id_char ,2);
          write( STDOUT_FILENO, "\n", 1 );
          continue;
        }

        sleep( id );                                    // while the philosopher is eating, he should wait for the mutex lock
        sem_wait( &mutex_lock );                        // once he have the lock, he should put the fork and knife back
        sem_post( &fork_and_knife[ fork ] );            // and release the mutex lock, resume thinking.
        sem_post( &fork_and_knife[ knife ] );
        sem_post( &mutex_lock );

        write( STDOUT_FILENO, "Philosopher Finished Eating, ID = ",34);
        write( STDOUT_FILENO, id_char, 2);
        write( STDOUT_FILENO, "\n", 1 );
      }
    }
  }
  exit( EXIT_SUCCESS );
}
