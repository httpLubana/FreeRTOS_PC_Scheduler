#include "scheduler.h"
#include <string.h>

// Tüm görevler burada tutulacak
static Task g_tasks[MAX_TASKS];
static int  g_task_count = 0;

// Giris.txt'den sırayla okumak için indeks
static int  g_next_arrival_index = 0;

// Kuyruklar
static Queue g_rt_queue;       // Gerçek zamanlı görev kuyruğu (öncelik 0)

static Queue g_user_queues[3]; // 3 seviyeli geri beslemeli kuyruk (öncelik 1,2,3)

// Şu an çalışan görev
static Task *g_current_task = NULL;

// Simülasyon zamanı
static int g_now = 0;

// Toplam görev sayısına göre simülasyonu bitirme kontrolü
static bool all_tasks_finished(void)
{
    for (int i = 0; i < g_task_count; ++i) {
        if (g_tasks[i].state != TASK_FINISHED) {
            return false;
        }
    }
    return true;
}

// Kuyruk yardımcıları
void queue_init(Queue *q)
{
    q->front = 0;
    q->rear  = -1;
    q->count = 0;
}

bool queue_is_empty(const Queue *q)
{
    return (q->count == 0);
}

bool queue_is_full(const Queue *q)
{
    return (q->count == MAX_QUEUE_SIZE);
}

bool queue_enqueue(Queue *q, int task_index)
{
    if (queue_is_full(q)) return false;
    q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
    q->items[q->rear] = task_index;
    q->count++;
    return true;
}

int queue_dequeue(Queue *q)
{
    if (queue_is_empty(q)) return -1;
    int idx = q->items[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    q->count--;
    return idx;
}

// giris.txt dosyasını oku
static bool load_tasks(const char *input_filename)
{
    FILE *f = fopen(input_filename, "r");
    if (!f) {
        perror("giris.txt acilamadi");
        return false;
    }

    int arrival, prio, time;
    int id_counter = 1;

    while (fscanf(f, "%d , %d , %d", &arrival, &prio, &time) == 3) {
        if (g_task_count >= MAX_TASKS) {
            fprintf(stderr, "Maksimum gorev sayisina ulasildi!\n");
            break;
        }
        Task *t = &g_tasks[g_task_count];
        t->id             = id_counter++;
        t->arrival_time   = arrival;
        t->base_priority  = prio;
        t->remaining_time = time;
        t->time_in_quantum= 0;
        t->state          = TASK_READY;
        t->color_index    = g_task_count % 6;  // birkaç renk döndür
        if (prio == 0) {
            t->current_queue = -1; // RT
        } else {
            // Kullanıcı görevleri için kuyruk indexi = prio-1 (0,1,2)
            t->current_queue = prio - 1;
        }
        g_task_count++;
    }

    fclose(f);
    return (g_task_count > 0);
}

// Belirli zamandaki yeni varan görevleri uygun kuyruklara at
static void handle_arrivals(void)
{
    // giris.txt den okurken sırayla dizimize attık; arrival_time'a göre
    // g_next_arrival_index ile zaman ilerledikçe kontrol ediyoruz.
    while (g_next_arrival_index < g_task_count &&
           g_tasks[g_next_arrival_index].arrival_time == g_now) {

        Task *t = &g_tasks[g_next_arrival_index];
        if (t->base_priority == 0) {
            // Gerçek zamanlı görev
            queue_enqueue(&g_rt_queue, g_next_arrival_index);
        } else {
            // Kullanıcı görevi -> ilgili geri besleme kuyruğuna
            int q_index = t->current_queue; // 0,1,2
            if (q_index < 0 || q_index > 2) q_index = 2;
            queue_enqueue(&g_user_queues[q_index], g_next_arrival_index);
        }
        g_next_arrival_index++;
    }
}

// Bir sonraki çalışacak görevi seç
static void select_next_task(void)
{
    // Eğer şu an çalışan yoksa yeni seçeceğiz
    if (g_current_task == NULL) {
        // Önce RT kuyruğuna bak
        if (!queue_is_empty(&g_rt_queue)) {
            int idx = queue_dequeue(&g_rt_queue);
            g_current_task = &g_tasks[idx];

            bool first_start = (g_current_task->state != TASK_RUNNING);
            g_current_task->state = TASK_RUNNING;
            g_current_task->time_in_quantum = 0;
            if (first_start) {
                task_log_start(g_current_task, g_now);
            } else {
                task_log_resume(g_current_task, g_now);
            }
            return;
        }

        // Kullanıcı görevleri için: en yüksek öncelikli dolu kuyruk
        for (int level = 0; level < 3; ++level) {
            if (!queue_is_empty(&g_user_queues[level])) {
                int idx = queue_dequeue(&g_user_queues[level]);
                g_current_task = &g_tasks[idx];

                bool first_start = (g_current_task->state != TASK_RUNNING);
                g_current_task->state = TASK_RUNNING;
                g_current_task->time_in_quantum = 0;
                if (first_start) {
                    task_log_start(g_current_task, g_now);
                } else {
                    task_log_resume(g_current_task, g_now);
                }
                return;
            }
        }

        // Hiç görev yoksa current_task NULL kalır (idle)
    }
}

// Şu an kullanıcı görevi çalışıyorken RT kuyruğunda iş varsa preempt et
static void maybe_preempt_by_rt(void)
{
    if (g_current_task == NULL) return;
    if (g_current_task->base_priority == 0) return; // Zaten RT
    if (queue_is_empty(&g_rt_queue)) return;        // RT beklemiyor

    // Kullanıcı görevi RT tarafından kesilecek
    g_current_task->state = TASK_SUSPENDED;

    int old_level = g_current_task->current_queue;
    int new_level = old_level;
    if (new_level < 2) new_level++;
    g_current_task->current_queue = new_level;

    task_log_suspend(g_current_task, g_now, new_level);
    queue_enqueue(&g_user_queues[new_level], g_current_task - g_tasks);
    g_current_task = NULL;
}

// Bir saniyelik çalışma simülasyonu
static void run_one_tick(void)
{
    // 1) Önce varışları işle
    handle_arrivals();

    // 2) Kullanıcı görevi çalışıyorsa ve RT kuyruğunda iş varsa preempt et
    maybe_preempt_by_rt();

    // 3) Eğer şu an görev yoksa uygun görevi seç
    if (g_current_task == NULL) {
        select_next_task();
    }

    // 4) Çalışan görev varsa 1 saniye çalıştır
    if (g_current_task != NULL) {
        task_log_tick(g_current_task, g_now);

        g_current_task->remaining_time--;
        g_current_task->time_in_quantum++;

        if (g_current_task->base_priority == 0) {
            // RT görev: süre bitene kadar kesilmez, quantum yok
            if (g_current_task->remaining_time <= 0) {
                g_current_task->state = TASK_FINISHED;
                task_log_finish(g_current_task, g_now);
                g_current_task = NULL;
            }
        } else {
            // Kullanıcı görevi: quantum = 1 saniye
            if (g_current_task->remaining_time <= 0) {
                g_current_task->state = TASK_FINISHED;
                task_log_finish(g_current_task, g_now);
                g_current_task = NULL;
            } else if (g_current_task->time_in_quantum >= 1) {
                // Quantum bitti, askıya alıp daha düşük öncelikli kuyruğa gönder
                g_current_task->state = TASK_SUSPENDED;
                int old_level = g_current_task->current_queue;
                int new_level = old_level;
                if (new_level < 2) new_level++;
                g_current_task->current_queue = new_level;
                task_log_suspend(g_current_task, g_now, new_level);
                queue_enqueue(&g_user_queues[new_level], g_current_task - g_tasks);
                g_current_task = NULL;
            }
        }
    } else {
        // 5) Hiç görev yoksa idle çıktısı
        printf("[t=%2d] Sistem bos (IDLE)\n", g_now);
    }

    // 6) Zamanı ilerlet
    g_now++;
}

// Dışarıdan çağrılacak fonksiyonlar
bool scheduler_init(const char *input_filename)
{
    g_task_count = 0;
    g_next_arrival_index = 0;
    g_current_task = NULL;
    g_now = 0;

    queue_init(&g_rt_queue);
    for (int i = 0; i < 3; ++i) {
        queue_init(&g_user_queues[i]);
    }

    return load_tasks(input_filename);
}

void scheduler_run(void)
{
    printf("=== FreeRTOS Benzeri 4 Seviyeli Scheduler Simulasyonu Basladi ===\n");

    // Simülasyonu, tüm görevler bitene kadar döndür
    while (!all_tasks_finished()) {
        run_one_tick();
    }

    printf("=== TUM GOREVLER TAMAMLANDI, SIMULASYON BITTI ===\n");

    // >>> HOCANIN ISTEDIGI SIMULASYON OZETI BURADA <<<
    int completed = 0;
    for (int i = 0; i < g_task_count; ++i) {
        if (g_tasks[i].state == TASK_FINISHED) {
            completed++;
        }
    }

    printf("\n----------------------------------------\n");
    printf("SIMULASYON OZETI:\n");
    printf("Tamamlanan gorev sayisi : %d\n", completed);
    printf("Toplam gecen sure       : %d saniye\n", g_now);
    printf("----------------------------------------\n\n");
}

void scheduler_cleanup(void)
{
    // Şimdilik dinamik bellek kullanmadigimiz icin bos
}
