#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define TOTAL_STUDENTS 300
#define ROOM_CAPACITY 30
#define TOTAL_ROOMS (TOTAL_STUDENTS / ROOM_CAPACITY)

sem_t room_sem[TOTAL_ROOMS];   // Semaphore per room
pthread_mutex_t lock;          // Mutex for controlling leaving
int room_allocation[TOTAL_STUDENTS];  // Room assigned to each student

// Student thread function
void* student_routine(void* arg) {
    int id = (int)arg;
    int room = room_allocation[id];

    // Wait for exam start
    sem_wait(&room_sem[room]);

    printf("Student %d entered Room %d\n", id, room);

    // Exam in progress...
    sleep(3);

    // Mutex to prevent early leaving
    pthread_mutex_lock(&lock);
    printf("Student %d leaving Room %d\n", id, room);
    pthread_mutex_unlock(&lock);

    sem_post(&room_sem[room]);  // Free seat
    pthread_exit(NULL);
}

int main() {
    pthread_t students[TOTAL_STUDENTS];
    int ids[TOTAL_STUDENTS];

    // Initialize semaphores for rooms (capacity = ROOM_CAPACITY)
    for (int i = 0; i < TOTAL_ROOMS; i++) {
        sem_init(&room_sem[i], 0, ROOM_CAPACITY);
    }
    pthread_mutex_init(&lock, NULL);

    // Allocate students to rooms
    for (int i = 0; i < TOTAL_STUDENTS; i++) {
        ids[i] = i + 1;
        room_allocation[i] = i / ROOM_CAPACITY;  // Room assignment
    }

    // Create student threads
    for (int i = 0; i < TOTAL_STUDENTS; i++) {
        pthread_create(&students[i], NULL, student_routine, &ids[i]);
    }

    // Wait for all students
    for (int i = 0; i < TOTAL_STUDENTS; i++) {
        pthread_join(students[i], NULL);
    }

    // Summary
    printf("\n=== Exam Summary ===\n");
    for (int i = 0; i < TOTAL_ROOMS; i++) {
        printf("Room %d: %d students\n", i, ROOM_CAPACITY);
    }

    pthread_mutex_destroy(&lock);
    for (int i = 0; i < TOTAL_ROOMS; i++) {
        sem_destroy(&room_sem[i]);
    }

    return 0;
}
