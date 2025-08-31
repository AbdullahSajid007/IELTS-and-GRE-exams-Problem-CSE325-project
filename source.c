/*
 * Mock IELTS & GRE Exam Manager
 * --------------------------------
 * This program simulates the management of students entering exam rooms, 
 * taking an exam, and leaving after it ends. 
 *
 * Features:
 *  - Uses fork() and pipe() for inter-process communication (IPC).
 *  - Uses pthreads for simulating multiple students concurrently.
 *  - Synchronization is handled with semaphores, mutexes, and condition variables.
 *
 * Scenario:
 *  - NUM_STUDENTS students need to attend an exam.
 *  - Students are distributed across NUM_ROOMS with ROOM_CAPACITY seats each.
 *  - All students must enter before the exam starts.
 *  - When the exam ends (signaled by condition variable), all students leave.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>

/* ------------ Configurable parameters ------------ */
#define NUM_STUDENTS   300         // Total number of students
#define ROOM_CAPACITY  30          // Maximum capacity per exam room
#define NUM_ROOMS ((NUM_STUDENTS+ROOM_CAPACITY - 1)/ROOM_CAPACITY) 
                                   // Total rooms required (ceiling division)

/* ------------ Data structures ------------ */

// Represents a student
typedef struct {
    int id;        // Unique student ID
    int room_id;   // Room assigned
} Student;

// Represents an exam room
typedef struct {
    int id;         // Room number
    int capacity;   // Maximum allowed capacity
} Room;

/* ------------ Global data ------------ */
static Student students[NUM_STUDENTS];      // Array of all students
static Room rooms[NUM_ROOMS];               // Array of rooms
static int room_attendance[NUM_ROOMS];      // Tracks how many students are inside each room

/* ------------ Synchronization primitives ------------ */
static sem_t exam_gate;                     // Gate controlling student entry
static pthread_mutex_t room_mutex = PTHREAD_MUTEX_INITIALIZER; // Protects room_attendance
static pthread_mutex_t exam_mutex = PTHREAD_MUTEX_INITIALIZER; // Protects exam_over flag
static pthread_cond_t end_bell = PTHREAD_COND_INITIALIZER;     // Signals exam end
static int exam_over = 0;                   // Flag to signal exam completion

// Struct for passing arguments to student threads
typedef struct {
    int student_id;
    int room_id;
} Thread_student;

/* ------------ Student thread function ------------ */
/*
 * Each student waits for the exam gate to open (exam start),
 * then enters the assigned room, waits until the exam is over,
 * and finally leaves the room.
 */
void* student_thread(void *arg_void) {
    Thread_student *student = (Thread_student *)arg_void;

    // Wait until exam starts
    sem_wait(&exam_gate);

    // Enter room (protected by mutex to update attendance safely)
    pthread_mutex_lock(&room_mutex);
    room_attendance[student->room_id]++;
    int count = room_attendance[student->room_id];

    // Safety check: detect over-capacity
    if (count > ROOM_CAPACITY)
       printf("ERROR: Room %d over capacity! count=%d (student %d)\n",
              student->room_id + 1, count, student->student_id);

    printf("Student %3d entered Room %2d\n", 
            student->student_id, student->room_id + 1);
    pthread_mutex_unlock(&room_mutex);

    // Wait until exam is declared over
    pthread_mutex_lock(&exam_mutex);
    while (!exam_over)
        pthread_cond_wait(&end_bell, &exam_mutex);
    pthread_mutex_unlock(&exam_mutex);

    // Student leaves room
    printf("Student %3d left Room %2d\n", student->student_id, student->room_id + 1);
    free(student);
    return NULL;
}

/* ------------ Child process function ------------ */
/*
 * This function is run by the child process after fork().
 * It assigns room IDs to all students (simple division-based allocation),
 * then sends the assignments back to the parent via pipe().
 */
static void child_allocate_and_send(int write_fd) {
    int *room_ids = malloc(sizeof(int) * NUM_STUDENTS);
    if (!room_ids) _exit(1);

    // Assign room IDs: students evenly distributed
    for (int i = 0; i < NUM_STUDENTS; i++)
        room_ids[i] = i / ROOM_CAPACITY;

    // Send room assignments to parent process
    write(write_fd, room_ids, sizeof(int) * NUM_STUDENTS);

    free(room_ids);
    close(write_fd);
    _exit(0);  // Exit child process
}

/* ------------ Main function ------------ */
int main() {
    printf("Mock IELTS & GRE Exam Manager\n");
    printf("Students: %d | Rooms: %d | Capacity/Room: %d\n\n",
           NUM_STUDENTS, NUM_ROOMS, ROOM_CAPACITY);

    /* --- Setup IPC using pipe and fork --- */
    int readWrite[2];
    if (pipe(readWrite) == -1) {
        perror("pipe"); exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork"); exit(1);
    }

    // Child: allocate and send room IDs
    if (pid == 0) {
        close(readWrite[0]); // Close unused read end
        child_allocate_and_send(readWrite[1]);
    }

    // Parent: receive room assignments
    close(readWrite[1]);
    int room_ids_buf[NUM_STUDENTS];
    read(readWrite[0], room_ids_buf, sizeof(int) * NUM_STUDENTS);
    close(readWrite[0]);
    wait(NULL);  // Wait for child to finish

    /* --- Initialize rooms and students --- */
    for (int r = 0; r < NUM_ROOMS; r++) {
        rooms[r].id = r;
        rooms[r].capacity = ROOM_CAPACITY;
        room_attendance[r] = 0;
    }
    for (int i = 0; i < NUM_STUDENTS; i++) {
        students[i].id = i + 1;              // Student IDs start from 1
        students[i].room_id = room_ids_buf[i];
    }

    sem_init(&exam_gate, 0, 0);

    /* --- Create student threads --- */
    pthread_t thread_id[NUM_STUDENTS];
    for (int i = 0; i < NUM_STUDENTS; i++) {
        Thread_student *arg = malloc(sizeof(Thread_student));
        arg->student_id = students[i].id;
        arg->room_id = students[i].room_id;
        pthread_create(&thread_id[i], NULL, student_thread, arg);
    }

    /* --- Simulate exam start --- */
    usleep(150 * 1000); // Small delay before starting exam
    printf("\n=== EXAM STARTED ===\n");

    // Allow all students to enter
    for (int i = 0; i < NUM_STUDENTS; i++) {
        sem_post(&exam_gate);
    }

    sleep(3); // Simulated exam duration

    /* --- Exam end signal --- */
    pthread_mutex_lock(&exam_mutex);
    exam_over = 1;
    pthread_cond_broadcast(&end_bell);
    pthread_mutex_unlock(&exam_mutex);
    printf("=== EXAM ENDED ===\n\n");

    /* --- Wait for all students to finish --- */
    for (int i = 0; i < NUM_STUDENTS; i++) {
        pthread_join(thread_id[i], NULL);
    }

    sem_destroy(&exam_gate);

    /* --- Print summary report --- */
    printf("---------- SUMMARY ----------\n");
    int total = 0;
    for (int r = 0; r < NUM_ROOMS; r++) {
        int c = room_attendance[r];
        total += c;
        printf("Room %2d: %2d students (capacity %d)\n",
               r + 1, c, rooms[r].capacity);
        if (c > rooms[r].capacity)
            printf("  WARNING: over capacity by %d!\n", c - rooms[r].capacity);
    }
    printf("-----------------------------\n");
    printf("Total attended: %d / %d\n", total, NUM_STUDENTS);

    return 0;
}
