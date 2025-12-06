CC=gcc
CFLAGS=-pthread
TARGET=psum
LIB_NAME=libparallel_sum.a

LIB_SOURCES=parallel_sum_lib.c utils.c
MAIN_SOURCE=parallel_sum.c
OBJECTS=$(LIB_SOURCES:.c=.o)

all: $(TARGET)

$(LIB_NAME): $(OBJECTS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(LIB_NAME) $(MAIN_SOURCE)
	$(CC) $(CFLAGS) -o $@ $(MAIN_SOURCE) -L. -lparallel_sum

clean:
	rm -f $(TARGET) $(LIB_NAME) $(OBJECTS)

rebuild: clean all
