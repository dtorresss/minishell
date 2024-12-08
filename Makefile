# Variables
CC = gcc
CFLAGS = -Wall -g -O0  # -g para depuración y -O0 para desactivar optimización
LIBS = -L. -lparser  # Se busca la librería libparser.a en el directorio actual
SRC = myshell.c  # Asegúrate de incluir tus fuentes aquí
OUT = myshell

# Regla para compilar el ejecutable
$(OUT): $(SRC) libparser.a  # Añadimos libparser.a explícitamente
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LIBS) -static

# Regla por defecto (all)
all: $(OUT)

# Regla para limpiar los archivos generados
clean:
	rm -f $(OUT)
