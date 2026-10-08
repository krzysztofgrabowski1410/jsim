TARGET_NAME = libma.so
APP_NAME = moore_app
CC = gcc
C_FLAGS = -Wall -Wextra -Wno-implicit-fallthrough -std=gnu17 -fPIC -O2
LD_FLAGS = -shared -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc -Wl,--wrap=reallocarray -Wl,--wrap=free -Wl,--wrap=strdup -Wl,--wrap=strndup
C_SRC = ma.c connection.c memory_tests.c
C_OBJ = $(patsubst %.c, %.o, $(C_SRC))

.PHONY: all clean run

all: $(TARGET_NAME) $(APP_NAME)

$(TARGET_NAME): $(C_OBJ)
	$(CC) $(C_FLAGS) -o $@ $^ $(LD_FLAGS)

$(APP_NAME): ma_example.c $(TARGET_NAME)
	$(CC) $(C_FLAGS) -o $@ $^ -L. -lma

clean:
	rm -rf $(TARGET_NAME) $(APP_NAME) *.o

run: $(APP_NAME)
	./$(APP_NAME)

%.o: %.c
	$(CC) $(C_FLAGS) -c $< -o $@