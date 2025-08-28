#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_STUDENTS   300
#define ROOM_CAPACITY  30
#define NUM_ROOMS ((NUM_STUDENTS + ROOM_CAPACITY - 1) / ROOM_CAPACITY)

typedef struct {
    int id;
    int room_id;
} Student;

typedef struct {
    int id;
    int capacity;
} Room;

/* ------------ Global data ------------ */
static Student students[NUM_STUDENTS];
static Room rooms[NUM_ROOMS];
static int room_attendance[NUM_ROOMS];

static sem_t exam_gate;
static pthread_mutex_t room_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t end_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t end_cv = PTHREAD_COND_INITIALIZER;
static int exam_over = 0;

typedef struct {
    int sid;
    int room_id;
} ThreadArg;

/* ------------ Student thread function ------------ */
void* student_thread(void *arg_void) {
    ThreadArg *arg = (ThreadArg*)arg_void;

    // Wait for exam start
    sem_wait(&exam_gate);

    // Enter room safely
    pthread_mutex_lock(&room_mutex);
    room_attendance[arg->room_id]++;
    int count = room_attendance[arg->room_id];
    if (count > ROOM_CAPACITY)
        fprintf(stderr, "ERROR: Room %d over capacity! count=%d (student %d)\n",arg->room_id + 1, count, arg->sid);

    printf("Student %3d entered Room %2d\n", arg->sid, arg->room_id + 1);
    pthread_mutex_unlock(&room_mutex);

    // Wait until exam ends
    pthread_mutex_lock(&end_mutex);
    while (!exam_over)
        pthread_cond_wait(&end_cv, &end_mutex);
    pthread_mutex_unlock(&end_mutex);

    printf("Student %3d left Room %2d\n", arg->sid, arg->room_id + 1);
    free(arg);
    return NULL;
}

/* ------------ Child process: allocate students ------------ */
static void child_allocate_and_send(int write_fd) {
    int *room_ids = malloc(sizeof(int) * NUM_STUDENTS);
    if (!room_ids) _exit(1);

    for (int i = 0; i < NUM_STUDENTS; i++)
        room_ids[i] = i / ROOM_CAPACITY; // Assign room

    write(write_fd, room_ids, sizeof(int) * NUM_STUDENTS);
    free(room_ids);
    close(write_fd);
    _exit(0);
}

/* ------------ Main function ------------ */
int main(void) {
    printf("Mock IELTS & GRE Exam Manager\n");
    printf("Students: %d | Rooms: %d | Capacity/Room: %d\n\n",
           NUM_STUDENTS, NUM_ROOMS, ROOM_CAPACITY);

    int fds[2];
    if (pipe(fds) == -1) { perror("pipe"); exit(1); }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0) {
        close(fds[0]);
        child_allocate_and_send(fds[1]);
    }

    // Parent: read allocation
    close(fds[1]);
    int room_ids_buf[NUM_STUDENTS];
    read(fds[0], room_ids_buf, sizeof(int) * NUM_STUDENTS);
    close(fds[0]);
    wait(NULL);

    // Initialize students & rooms
    for (int r = 0; r < NUM_ROOMS; r++) {
        rooms[r].id = r;
        rooms[r].capacity = ROOM_CAPACITY;
        room_attendance[r] = 0;
    }
    for (int i = 0; i < NUM_STUDENTS; i++) {
        students[i].id = i + 1;
        students[i].room_id = room_ids_buf[i];
    }

    sem_init(&exam_gate, 0, 0);

    // Create student threads
    pthread_t tids[NUM_STUDENTS];
    for (int i = 0; i < NUM_STUDENTS; i++) {
        ThreadArg *arg = malloc(sizeof(ThreadArg));
        arg->sid = students[i].id;
        arg->room_id = students[i].room_id;
        pthread_create(&tids[i], NULL, student_thread, arg);
    }

    usleep(150 * 1000);
    printf("\n=== EXAM STARTED ===\n");
    for (int i = 0; i < NUM_STUDENTS; i++) sem_post(&exam_gate);

    sleep(3); // simulate 3-hour exam

    // End exam
    pthread_mutex_lock(&end_mutex);
    exam_over = 1;
    pthread_cond_broadcast(&end_cv);
    pthread_mutex_unlock(&end_mutex);
    printf("=== EXAM ENDED ===\n\n");

    for (int i = 0; i < NUM_STUDENTS; i++) pthread_join(tids[i], NULL);
    sem_destroy(&exam_gate);

    // Summary
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


//need to note down the output