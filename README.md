# 📚 Library Management System (Reader-Writer Problem)

This C++ project is a simulation of the classic **Reader-Writer Problem**, implemented for the **Operating Systems** course. It mimics a real-world **Library Management System** where multiple reader and writer threads interact with shared data — enforcing proper synchronization using mutexes and semaphores.


## 🎯 Objective

- Implement Reader-Writer synchronization with **mutex** and **semaphores**
- Allow **multiple readers** to read shared data concurrently
- Ensure **exclusive access** for writers while updating data
- Prevent race conditions and ensure thread-safe data access


## 🧵 Synchronization Overview

### ✅ Reader Behavior

- Locks mutex to update `reader_count`
- If first reader → blocks writers
- Reads shared data
- After reading, decrements `reader_count`
- If last reader → allows writers

### ✍️ Writer Behavior

- Waits for exclusive access using `write_block`
- Writes to shared resource
- Releases `write_block` after writing

### 🧰 Tools Used

- `mutex` → Protects shared counters like `reader_count`
- `semaphores` → Manages exclusive write access (`write_block`)
- `stats_mutex` → Tracks total reads and writes safely


## 🧪 Features

- Threaded simulation of readers and writers accessing the library
- Display and manage a list of books
- Track total number of reads and writes
- Safe concurrent access using synchronization primitives
- Manual testing with randomized thread behavior


## 💻 How to Run

1. **Compile**
   ```bash
   g++ -pthread library_management.cpp -o library

2. **Run**
   ./library

3. **Interact through Menu**

- Start simulation
- Add/Remove books manually
- View current book list
- Display access statistics


## 🧪 Reader-Writer Simulation Example

To understand how synchronization works in this system, here's a simple dry run with 2 readers and 1 writer:

| 🧭 Event           | 📚 Reader Count | ⚙️ Action                    |
|-------------------|------------------|------------------------------|
| Reader1 enters    | 1                | First reader → blocks writers |
| Reader2 enters    | 2                | Continues reading             |
| Writer tries      | 2                | Writer is blocked             |
| Reader1 leaves    | 1                | Still reading                 |
| Reader2 leaves    | 0                | Last reader → allows writer   |
| Writer writes     | 0                | Writer has exclusive access   |

This illustrates how:
- Multiple readers can read simultaneously
- Writers wait until all readers leave
- Synchronization ensures no data corruption or race condition


## 📘 Learning Outcomes

- Gained experience with **multithreading** in C++
- Understood and implemented the **Reader-Writer synchronization pattern**
- Learned to use **mutexes and semaphores** to prevent race conditions
- Designed and tracked real-time shared data access statistics
- Simulated classical OS-level synchronization problems practically


## 🔮 Future Improvements

- Add **fair scheduling** to prevent starvation
- Implement **real-time logging** of reader/writer actions
- Introduce **file-based persistence** for book data
- Build a **GUI interface** to visualize read/write activity
- Add support for **reader/writer priority modes**

---

## 📃 License

This project is intended for academic and educational purposes.

