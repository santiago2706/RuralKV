# 📘 Manual de Uso: RuralKV Engine

Este documento documenta la capa a bajo nivel (escrita en C) del proyecto **RuralKV**. Aquí verás cómo compilar el motor sin depender del programa `make` y cómo invocar sus funciones principales si tú quisieras conectarle otra interfaz.

## 1. Compilación Fácil (Especial para Windows)

Ya que nos dimos cuenta de que PowerShell no reconoce el comando Unix `make`, la forma más pura y profesional de compilar este proyecto usando directamente los binarios que genera `gcc` es la siguiente:

Abre tu terminal en tu carpeta base (`d:\DOCUMENTOS\3ER_SEMESTRE\LP1\PROYECTO_FERIA\RuralKV\RuralKV`) y pega esto:

```bash
gcc -Wall -Wextra -pthread -std=c11 -O2 -I./include src/*.c -o ruralkv.exe
```

Ese comando "atrapa" todos los archivos de tu carpeta `src/`, les liga las cabeceras de `include/` y genera el motor final en un ejecutable llamado `ruralkv.exe`.

Para correr tu motor de prueba, usa:
```bash
.\ruralkv.exe
```

---

## 2. API C del Motor (Documentación de Funciones)

Si un programador mira el código de tu C, esto es lo que tiene a su disposición como librería:

### A. Módulo RAM Salvavidas: `arena.h`
Evita el `malloc()` desproporcionado. Limita el sistema para que nunca exceda los megabytes asignados.

*   `Arena* arena_init(size_t capacity)`: Crea la burbuja de memoria general protegida.
    *   *Uso:* `Arena* a = arena_init(1024 * 1024); // Da 1 MB de vida util`
*   `void* arena_alloc(Arena* arena, size_t size)`: Pide bytes desde la Arena a velocidad O(1).
*   `void arena_free(Arena* arena)`: Regresa todo el megabyte físico devuelta al SO de Windows/Linux. (Solo usar antes del `return 0;`).

### B. Módulo Búsqueda Mágica: `hash.h`
Tabla clave-valor que encripta llaves en índices enteros usando el algoritmo `djb2`. Todo esto sacando recursos del Arena Allocator.

*   `HashTable* hash_create(Arena* arena, size_t size)`: Genera el casillero.
*   `bool hash_put(HashTable* ht, char* key, char* value)`: Guarda datos a máxima velocidad superponiéndose automáticamente encima del viejo valor si la llave ya existía.
*   `const char* hash_get(HashTable* ht, char* key)`: Consulta base de datos (`O(1)` teóricamente).

### C. Módulo Escudo Anti-Apagones: `wal.h`
El Write-Ahead Log. Lo que la Memoria olvida, el Disco lo recuerda.

*   `Wal* wal_init(const char* filepath)`: Levanta o crea un archivo de logs que sólo permite anexar al final (`Append`).
*   `bool wal_append_put(Wal* wal, char* key, char* value)`: Graba ininterrumpidamente al disco de estado sólido, burlando cachés con la directriz nativa `fflush()`.
*   `void wal_close(Wal* wal)`: Bloqueo seguro de I/O sobre archivos de Windows.

---

## 3. Regla del Protocolo de Escritura

Como Arquitecto de Software, cuando llegue un nuevo dato de la interfaz web, tu programa en C siempre debe acatar el **Dogma RuralKV**:

1.  **WAL Primero:** `wal_append_put(wal, ...)`
    *   *El dato primero debe sobrevivir al apocalipsis eléctrico tocando disco físico.*
2.  **RAM Después:** `hash_put(db, ...)`
    *   *Una vez que hay supervivencia garantizada, el dato ingresa a la RAM para ser leído a velocidad luz.*
