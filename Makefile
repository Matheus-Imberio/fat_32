# Variáveis para compilação
CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = 

# Nome do executável
TARGET = fat32

# Arquivos fonte
SRCS = main.c
OBJS = $(SRCS:.c=.o)

# Regra padrão para compilar tudo
all: $(TARGET)

# Como criar o executável
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Como compilar os arquivos .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpa os arquivos gerados
clean:
	rm -f $(OBJS) $(TARGET)