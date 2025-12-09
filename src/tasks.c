#include "scheduler.h"

// Basit ANSI renk kodlari 
static const char *colors[] = {
    "\x1b[31m", // kirmizi
    "\x1b[32m", // yesil
    "\x1b[33m", // sari
    "\x1b[34m", // mavi
    "\x1b[35m", // mor
    "\x1b[36m"  // camgobek
};
static const char *color_reset = "\x1b[0m";

static const char *get_color(int idx)
{
    int n = (int)(sizeof(colors) / sizeof(colors[0]));
    if (idx < 0) idx = 0;
    return colors[idx % n];
}

// ---- SADECE BURASI DEĞİŞTİ ----
// Zamanı tam senin istediğin formatta yazar:  X.0000 sn
static void print_time_ms(int t)
{
    printf("%d.0000 sn", t);
}

// --------------------------------
// Görev başlarken
void task_log_start(Task *task, int now)
{
    const char *c = get_color(task->color_index);

    printf("%s", c);
    print_time_ms(now);

    if (task->base_priority == 0) {  
        // RT görev
        printf("Gorev #%d BASLADI (id:%04d, oncelik=0, kalan sure=%d s)%s\n",
               task->id, task->id, task->remaining_time, color_reset);
    } 
    else {  
        // Kullanıcı görevi
        printf("Gorev #%d BASLADI (id:%04d, kullanici oncelik=%d, kuyruk=%d, kalan=%d s)%s\n",
               task->id, task->id, task->base_priority, task->current_queue,
               task->remaining_time, color_reset);
    }
}


// Her saniye çalışırken
void task_log_tick(Task *task, int now)
{
    const char *c = get_color(task->color_index);
    printf("%s", c);
    print_time_ms(now);
    printf(" Gorev #%d CALISIYOR (oncelik=%d, kuyruk=%d, kalan=%d s)%s\n",
           task->id, task->base_priority, task->current_queue,
           task->remaining_time, color_reset);
}

// Askıya alınırken
void task_log_suspend(Task *task, int now, int new_queue_level)
{
    const char *c = get_color(task->color_index);
    printf("%s", c);

    print_time_ms(now);

    if (task->base_priority == 0) {
        printf(" RT Gorev #%d ASKIDA / BITTI (kalan=%d s)%s\n",
               task->id, task->remaining_time, color_reset);
    } else {
        printf(" Gorev #%d ASKIDA (yeni kuyruk=%d, kalan=%d s)%s\n",
               task->id, new_queue_level, task->remaining_time, color_reset);
    }
}

// Devam ederken
void task_log_resume(Task *task, int now)
{
    const char *c = get_color(task->color_index);
    printf("%s", c);
    print_time_ms(now);
    printf(" Gorev #%d DEVAM EDIYOR (oncelik=%d, kuyruk=%d, kalan=%d s)%s\n",
           task->id, task->base_priority, task->current_queue,
           task->remaining_time, color_reset);
}

// Bitince
void task_log_finish(Task *task, int now)
{
    const char *c = get_color(task->color_index);
    printf("%s", c);
    print_time_ms(now);
    printf(" Gorev #%d SONLANDI (oncelik=%d)%s\n",
           task->id, task->base_priority, color_reset);
}
