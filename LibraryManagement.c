#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#define MAX_BOOKS 10
#define MAX_READERS 10
#define MAX_WRITERS 5
#define MAX_TITLE_LENGTH 50
#define MAX_AUTHOR_LENGTH 30
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_WHITE   "\x1b[37m"


typedef struct {
    int id;
    char title[MAX_TITLE_LENGTH];
    char author[MAX_AUTHOR_LENGTH];
    int edition;
    int access_count;  
    int update_count;  
    bool active;       
} Book;

Book library[MAX_BOOKS];
int book_count = 0;

sem_t read_block;              
sem_t write_block;        
int reader_count = 0;     

pthread_t *readers;
pthread_t *writers;
int num_readers = 0;
int num_writers = 0;

volatile bool simulation_running = false;

int total_reads = 0;
int total_writes = 0;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

void initialize_library();
void display_all_books();
void *reader_function(void *arg);
void *writer_function(void *arg);
void update_book(int book_id, const char* new_title, const char* new_author, int new_edition);
void remove_book(int book_id);
void reader_lock();
void reader_unlock();
void writer_lock();
void writer_unlock();
void display_statistics();
void start_simulation(int sim_readers, int sim_writers, int duration);
void stop_simulation();
void cleanup_resources();

void initialize_library() {
    const char *titles[] = {
        "One Piece", 
        "Crime and Punishment", 
        "Harry Potter", 
        "Calculus Early Transcendentals",
        "Vinland Saga"
    };
    
    const char *authors[] = {
        "Eiichiro Oda", 
        "Fyodor Dostoevsky", 
        "J.K. Rowling", 
        "James Stewart",
        "Makoto Yukimura"
    };
    
    for (int i = 0; i < 5; i++) {
        library[i].id = i;
        strcpy(library[i].title, titles[i]);
        strcpy(library[i].author, authors[i]);
        library[i].edition = 1;
        library[i].access_count = 0;
        library[i].update_count = 0;
        library[i].active = true;
        book_count++;
    }
}

void display_all_books() {
    printf(ANSI_COLOR_RED);
    printf("\n===== LIBRARY CATALOG =====\n");
    int active_books = 0;
    
    for (int i = 0; i < MAX_BOOKS; i++) {
        if (library[i].active) {
            printf("\nBook ID: %d\n", library[i].id);
            printf("Title: %s\n", library[i].title);
            printf("Author: %s\n", library[i].author);
            printf("Edition: %d\n", library[i].edition);
            active_books++;
        }
    }
    
    if (active_books == 0) {
        printf("\nThe library is currently empty.\n");
    } else {
        printf("\nTotal books: %d\n", active_books);
    }
    
    printf("==========================\n");
}

void update_book(int book_id, const char* new_title, const char* new_author, int new_edition) {
    if (book_id >= 0 && book_id < MAX_BOOKS && library[book_id].active) {
        if (new_title != NULL) strcpy(library[book_id].title, new_title);
        if (new_author != NULL) strcpy(library[book_id].author, new_author);
        if (new_edition > 0) library[book_id].edition = new_edition;
        library[book_id].update_count++;
    }
}

void remove_book(int book_id) {
    if (book_id >= 0 && book_id < MAX_BOOKS && library[book_id].active) {
        library[book_id].active = false;
        printf("Book %d (%s) has been removed from the library\n", 
               book_id, library[book_id].title);
    }
}

void reader_lock() {
    sem_wait(&read_block);  
    reader_count++;
    if (reader_count == 1) {
        sem_wait(&write_block);
    }
    sem_post(&read_block);  
}

void reader_unlock() {
    sem_wait(&read_block);  
    reader_count--;
    if (reader_count == 0) {
        sem_post(&write_block);
    }
    sem_post(&read_block);  
}

void writer_lock() {
    sem_wait(&write_block);  
}

void writer_unlock() {
    sem_post(&write_block);  
}

int find_random_active_book() {
    int active_books[MAX_BOOKS];
    int count = 0;
    
    for (int i = 0; i < MAX_BOOKS; i++) {
        if (library[i].active) {
            active_books[count++] = i;
        }
    }
    
    if (count == 0) return -1;  
    
    return active_books[rand() % count];
}

void *reader_function(void *arg) {
    int reader_id = *((int *)arg);
    
    while (simulation_running) {
        int sleep_time = rand() % 3 + 1;
        sleep(sleep_time);
        
        if (!simulation_running) break;
        
        int book_id = find_random_active_book();
        if (book_id == -1) {
            printf("\nReader %d found no books available in the library!\n", reader_id);
            continue;
        }
        
        printf("\nReader %d wants to access book %d\n", reader_id, book_id);
        reader_lock();
        
        if (!simulation_running) {
            reader_unlock();
            break;
        }
        
        if (library[book_id].active) {
            printf("Reader %d is now reading book %d: '%s'\n", 
                   reader_id, book_id, library[book_id].title);
            
            library[book_id].access_count++;
            pthread_mutex_lock(&stats_mutex);
            total_reads++;
            pthread_mutex_unlock(&stats_mutex);
            
            sleep(1);
            
            printf("Reader %d finished reading book %d\n", reader_id, book_id);
        } else {
            printf("Reader %d found that book %d has been removed!\n", reader_id, book_id);
        }
        
        reader_unlock();
    }
    
    free(arg); 
    return NULL;
}

void *writer_function(void *arg) {
    int writer_id = *((int *)arg);
    
    while (simulation_running) {
        int sleep_time = rand() % 5 + 3;  
        sleep(sleep_time);
        
        if (!simulation_running) break;
        
        int book_id = find_random_active_book();
        if (book_id == -1) {
            printf("\nWriter %d found no books available to update!\n", writer_id);
            
            if (book_count < MAX_BOOKS) {
                writer_lock();
                
                if (!simulation_running) {
                    writer_unlock();
                    break;
                }
                
                int new_id = -1;
                for (int i = 0; i < MAX_BOOKS; i++) {
                    if (!library[i].active) {
                        new_id = i;
                        break;
                    }
                }
                
                if (new_id == -1) {
                    new_id = book_count;
                    if (new_id >= MAX_BOOKS) {
                        new_id = -1;  
                    }
                }
                
                if (new_id != -1) {
                    char new_title[MAX_TITLE_LENGTH];
                    sprintf(new_title, "New Book %d", new_id);
                    char new_author[MAX_AUTHOR_LENGTH];
                    sprintf(new_author, "Author %d", new_id);
                    
                    printf("\nWriter %d is adding a new book: '%s'\n", writer_id, new_title);
                    
                    library[new_id].id = new_id;
                    strcpy(library[new_id].title, new_title);
                    strcpy(library[new_id].author, new_author);
                    library[new_id].edition = 1;
                    library[new_id].access_count = 0;
                    library[new_id].update_count = 0;
                    library[new_id].active = true;
                    
                    if (new_id == book_count) {
                        book_count++;
                    }
                    
                    pthread_mutex_lock(&stats_mutex);
                    total_writes++;
                    pthread_mutex_unlock(&stats_mutex);
                    
                    printf("Writer %d added a new book with ID %d\n", writer_id, new_id);
                } else {
                    printf("Writer %d couldn't find a slot to add a new book!\n", writer_id);
                }
                
                writer_unlock();
            }
            
            continue;
        }
        
        printf("\nWriter %d (Librarian) wants to update book %d\n", writer_id, book_id);
        writer_lock();
        
        if (!simulation_running) {
            writer_unlock();
            break;
        }
        
        if (!library[book_id].active) {
            printf("Writer %d found that book %d has already been removed!\n", writer_id, book_id);
            writer_unlock();
            continue;
        }
    
        printf("Writer %d is now accessing book %d\n", writer_id, book_id);
        
        int update_type = rand() % 2;
        if (update_type == 0) {
            int new_edition = library[book_id].edition + 1;
            printf("Writer %d is updating book %d edition to %d\n", 
                   writer_id, book_id, new_edition);
            update_book(book_id, NULL, NULL, new_edition);
        } else {
            printf("Writer %d is removing book %d: '%s'\n", 
                   writer_id, book_id, library[book_id].title);
            remove_book(book_id);
        }
        
        pthread_mutex_lock(&stats_mutex);
        total_writes++;
        pthread_mutex_unlock(&stats_mutex);
        
        sleep(2);
        
        printf("Writer %d finished updating\n", writer_id);
        writer_unlock();
    }
    
    free(arg); 
    return NULL;
}

void display_statistics() {
    printf(ANSI_COLOR_YELLOW);
    printf("\n===== SYSTEM STATISTICS =====\n");
    printf("Total read operations: %d\n", total_reads);
    printf("Total write operations: %d\n", total_writes);
    printf("Active books in library: ");
    
    int active_count = 0;
    for (int i = 0; i < MAX_BOOKS; i++) {
        if (library[i].active) {
            active_count++;
        }
    }
    printf("%d\n", active_count);
    
    printf("\nIndividual Book Statistics:\n");
    for (int i = 0; i < MAX_BOOKS; i++) {
        if (library[i].active) {
            printf("Book %d (%s): %d reads, %d updates\n", 
                   i, library[i].title, library[i].access_count, library[i].update_count);
        }
    }
    
    printf("============================\n");
}

void start_simulation(int sim_readers, int sim_writers, int duration) {
    if (simulation_running) {
        printf("Simulation is already running!\n");
        return;
    }
    
    if (sim_readers <= 0 || sim_writers <= 0 || duration <= 0) {
        printf("Invalid parameters! Please use positive values.\n");
        return;
    }
    
    num_readers = sim_readers;
    num_writers = sim_writers;
    
    readers = (pthread_t*)malloc(num_readers * sizeof(pthread_t));
    writers = (pthread_t*)malloc(num_writers * sizeof(pthread_t));
    
    if (readers == NULL || writers == NULL) {
        printf("Memory allocation failed!\n");
        free(readers);
        free(writers);
        return;
    }
    
    simulation_running = true;
    printf(ANSI_COLOR_MAGENTA);
    printf("\nStarting simulation with %d readers and %d writers for %d seconds...\n", 
           num_readers, num_writers, duration);
    
    for (int i = 0; i < num_readers; i++) {
        int *id = (int*)malloc(sizeof(int));
        if (id == NULL) {
            printf("Memory allocation failed for reader ID!\n");
            continue;
        }
        *id = i;
        if (pthread_create(&readers[i], NULL, reader_function, id) != 0) {
            perror("Failed to create reader thread");
            free(id);
        }
    }
    
    for (int i = 0; i < num_writers; i++) {
        int *id = (int*)malloc(sizeof(int));
        if (id == NULL) {
            printf("Memory allocation failed for writer ID!\n");
            continue;
        }
        *id = i;
        if (pthread_create(&writers[i], NULL, writer_function, id) != 0) {
            perror("Failed to create writer thread");
            free(id);
        }
    }
    
    printf("Simulation will run for %d seconds...\n", duration);
    sleep(duration);
    
    stop_simulation();
}

void stop_simulation() {
    if (!simulation_running) {
        printf("No simulation is currently running!\n");
        return;
    }
    
    printf("\nStopping simulation...\n");
    simulation_running = false;
    
    for (int i = 0; i < num_readers; i++) {
        pthread_join(readers[i], NULL);
    }
    
    for (int i = 0; i < num_writers; i++) {
        pthread_join(writers[i], NULL);
    }
    
    free(readers);
    free(writers);
    
    printf("Simulation stopped. Displaying final statistics:\n");
    display_statistics();
}

void cleanup_resources() {
    if (simulation_running) {
        stop_simulation();
    }
    
    sem_destroy(&read_block);
    sem_destroy(&write_block);
    
    printf("\nResources cleaned up. Exiting program.\n");
}

void add_book_manually() {
    int new_id = -1;
    
    for (int i = 0; i < MAX_BOOKS; i++) {
        if (!library[i].active) {
            new_id = i;
            break;
        }
    }
    
    if (new_id == -1 && book_count < MAX_BOOKS) {
        new_id = book_count;
    }
    
    if (new_id == -1) {
        printf("Library is full, cannot add more books!\n");
        return;
    }
    
    char title[MAX_TITLE_LENGTH];
    char author[MAX_AUTHOR_LENGTH];
    
    printf("\nEnter book title: ");
    fgets(title, MAX_TITLE_LENGTH, stdin);
    title[strcspn(title, "\n")] = 0; 
    
    printf("Enter book author: ");
    fgets(author, MAX_AUTHOR_LENGTH, stdin);
    author[strcspn(author, "\n")] = 0; 
    
    library[new_id].id = new_id;
    strcpy(library[new_id].title, title);
    strcpy(library[new_id].author, author);
    library[new_id].edition = 1;
    library[new_id].access_count = 0;
    library[new_id].update_count = 0;
    library[new_id].active = true;
    
    printf("\nBook added successfully with ID: %d\n", new_id);
    
    if (new_id == book_count) {
        book_count++;
    }
}

void remove_book_manually() {
    display_all_books();
    
    int book_id;
    printf("\nEnter book ID to remove: ");
    scanf("%d", &book_id);
    getchar(); 
    
    if (book_id >= 0 && book_id < MAX_BOOKS && library[book_id].active) {
        writer_lock();  
        remove_book(book_id);
        writer_unlock();
    } else {
        printf("Invalid book ID or book already removed!\n");
    }
}

int display_menu() {
    int choice;
    
    printf(ANSI_COLOR_CYAN);
    printf("\n===== LIBRARY BOOK MANAGEMENT SYSTEM MENU =====\n");
    printf("1. Display all books\n");
    printf("2. Display statistics\n");
    printf("3. Start simulation\n");
    printf("4. Add new book manually\n");
    printf("5. Remove book manually\n");
    printf("6. Exit\n");
    printf("Enter your choice (1-6): ");
    
    scanf("%d", &choice);
    getchar(); 
    
    return choice;
}

int main() {
    srand(time(NULL));
    
    sem_init(&read_block, 0, 1);
    sem_init(&write_block, 0, 1);
    
    initialize_library();
    
    int choice;
    bool running = true;

    printf(ANSI_COLOR_WHITE);
    printf("\nWelcome to Library Management System! \n");
    
    while (running) {
        choice = display_menu();
        
        switch (choice) {
            case 1:
                display_all_books();
                break;
                
            case 2:
                display_statistics();
                break;
                
            case 3:
                if (!simulation_running) {
                    int sim_readers, sim_writers, duration;
                    
                    printf("\nEnter number of readers [1-%d]: ", MAX_READERS);
                    scanf("%d", &sim_readers);
                    
                    printf("Enter number of writers [1-%d]: ", MAX_WRITERS);
                    scanf("%d", &sim_writers);
                    
                    printf("Enter simulation duration (seconds): ");
                    scanf("%d", &duration);
                    getchar(); 
                    
                    if (sim_readers > 0 && sim_readers <= MAX_READERS &&
                        sim_writers > 0 && sim_writers <= MAX_WRITERS &&
                        duration > 0) {
                        start_simulation(sim_readers, sim_writers, duration);
                    } else {
                        printf("Invalid input! Please try again.\n");
                    }
                } else {
                    printf("Simulation is already running!\n");
                }
                break;
                
            case 4:
                add_book_manually();
                break;
                
            case 5:
                remove_book_manually();
                break;
                
            case 6:
                printf("\nExiting program...\n");
                running = false;
                break;
                
            default:
                printf("\nInvalid choice! Please try again.\n");
        }
    }
    
    cleanup_resources();
    return 0;
}