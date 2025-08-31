# Mock IELTS & GRE Exam Manager 🎓

This project simulates the management of a large-scale exam (IELTS/GRE style) using **C, POSIX threads, semaphores, mutexes, and inter-process communication**.

It demonstrates **process synchronization, concurrency control, and IPC** through a real-world inspired scenario:

* 300 students
* Rooms of capacity 30
* Students enter rooms, take the exam, and leave when the exam ends.

---

## ✨ Features

* ✅ **Multi-threading**: Each student is simulated by a thread.
* ✅ **Synchronization**: Uses **semaphores, mutexes, and condition variables**.
* ✅ **Inter-Process Communication (IPC)**: Child process allocates room IDs and sends them to parent using a pipe.
* ✅ **Over-capacity detection**: Warns if more students than capacity enter a room.
* ✅ **Detailed exam simulation log**: Tracks student entry, exam start/end, and summary.

---

## ⚙️ Requirements

* GCC (C compiler)
* POSIX Threads library (`pthread`)
* Linux/Unix-based OS (tested on Ubuntu)

---

## 🚀 How to Compile & Run

```bash
# Clone the repo
git clone https://github.com/your-username/mock-exam-manager.git
cd mock-exam-manager

# Compile
gcc exam_manager.c -o exam_manager -lpthread

# Run
./exam_manager
```

---

## 🧵 Synchronization Details

* **Semaphore (`exam_gate`)** → Blocks students until the exam officially starts.
* **Mutex (`room_mutex`)** → Protects shared `room_attendance` counter from race conditions.
* **Condition Variable (`end_bell`)** → Used to signal all students when the exam is over.
* **Pipe + Fork** → Child assigns students to rooms and sends results to parent process.

---

## 📊 Example Output

```
Mock IELTS & GRE Exam Manager
Students: 300 | Rooms: 10 | Capacity/Room: 30

Student   1 entered Room  1
Student   2 entered Room  1
...
=== EXAM STARTED ===
...
Student 300 left Room 10
=== EXAM ENDED ===

---------- SUMMARY ----------
Room  1: 30 students (capacity 30)
Room  2: 30 students (capacity 30)
...
Room 10: 30 students (capacity 30)
-----------------------------
Total attended: 300 / 300
```

---

## 📂 Project Structure

```
📦 mock-exam-manager
 ┣ 📜 exam_manager.c   # Main program source code
 ┣ 📜 README.md        # Project documentation
```

---

## 📜 License

This project is licensed under the MIT License.
You are free to use, modify, and distribute it.
