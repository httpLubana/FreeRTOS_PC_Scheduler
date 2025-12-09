#include "scheduler.h"

int main(int argc, char *argv[])
{
    const char *input_file = "giris.txt";

    if (argc >= 2) {
        input_file = argv[1];
    }

    if (!scheduler_init(input_file)) {
        fprintf(stderr, "Scheduler baslatilamadi. giris dosyasi: %s\n", input_file);
        return 1;
    }

    scheduler_run();
    scheduler_cleanup();

    return 0;
}
