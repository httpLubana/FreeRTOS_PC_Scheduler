# Derleyici ve seçenekler
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# Çıktı programının adı
TARGET = freertos_sim

# Proje kaynak dosyaları
SOURCES = src/main.c src/scheduler.c src/tasks.c

# Varsayılan hedef: programı derle
$(TARGET):
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

# Geçici dosyaları temizlemek için:
clean:
	rm -f $(TARGET)
