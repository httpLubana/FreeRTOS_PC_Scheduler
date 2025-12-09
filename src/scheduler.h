#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_TASKS       256
#define MAX_QUEUE_SIZE  256
#define MAX_NAME_LEN    32

// Görev durumları
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_SUSPENDED,
    TASK_FINISHED
} TaskState;

// Simüle edilen görev yapısı
typedef struct Task {
    int   id;                // Görev ID (1,2,3,...)
    int   arrival_time;      // Varış zamanı (saniye)
    int   base_priority;     // Orijinal öncelik (0=RT, 1,2,3=User seviyeleri)
    int   current_queue;     // Kullanıcı görevleri için şu anki kuyruk seviyesi (0,1,2) / RT için -1
    int   remaining_time;    // Kalan çalışma süresi (saniye)
    int   time_in_quantum;   // O anki quantum içinde harcanan süre (saniye)
    TaskState state;         // READY/RUNNING/SUSPENDED/FINISHED
    int   color_index;       // ANSI renk kodu seçimi için
} Task;

// Basit dairesel kuyruk
typedef struct Queue {
    int items[MAX_QUEUE_SIZE];   // Task ID veya indeks
    int front;
    int rear;
    int count;
} Queue;

// Scheduler fonksiyonları
bool scheduler_init(const char *input_filename);
void scheduler_run(void);
void scheduler_cleanup(void);

// Yardımcı kuyruk fonksiyonları (gerekirse main/scheduler içinde kullanılabilir)
void queue_init(Queue *q);
bool queue_is_empty(const Queue *q);
bool queue_is_full(const Queue *q);
bool queue_enqueue(Queue *q, int task_index);
int  queue_dequeue(Queue *q);

// Görev günlükleme (log) fonksiyonları - tasks.c içinde tanımlanacak
void task_log_start(Task *task, int now);
void task_log_tick(Task *task, int now);
void task_log_suspend(Task *task, int now, int new_queue_level);
void task_log_resume(Task *task, int now);
void task_log_finish(Task *task, int now);

#endif // SCHEDULER_H
