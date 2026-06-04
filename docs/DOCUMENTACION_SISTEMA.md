# Documentacion del Sistema RuralKV

## 1. Vision general

RuralKV es un motor de almacenamiento clave-valor ligero, escrito en C, pensado para funcionar como una base local simple en entornos con pocos recursos. El proyecto apunta al caso de uso de ImmortalData: una solucion para registrar informacion de salud en zonas rurales, donde la conexion a internet puede ser intermitente o inexistente.

En el estado actual del repositorio, RuralKV implementa:

- Un gestor de memoria tipo Arena Allocator.
- Una tabla hash en memoria para almacenar pares clave-valor.
- Un Write-Ahead Log simple para persistir operaciones `PUT`.
- Un servidor HTTP nativo en C con rutas basicas `/put` y `/get`.
- Un cliente interactivo en Python que se comunica con el servidor.
- Scripts de compilacion mediante `Makefile` y comando directo con `gcc`.

Algunas capacidades mencionadas en la vision del proyecto, como sincronizacion Delta Sync, Pub/Sub, interfaz HTMX/Alpine.js, autenticacion, recuperacion automatica desde WAL y concurrencia real por hilos, todavia no aparecen implementadas en el codigo actual.

## 2. Estructura del proyecto

```text
RuralKV/
|-- include/
|   |-- arena.h
|   |-- hash.h
|   |-- server.h
|   `-- wal.h
|-- src/
|   |-- arena.c
|   |-- hash.c
|   |-- main.c
|   |-- server.c
|   `-- wal.c
|-- docs/
|   |-- MANUAL.md
|   `-- DOCUMENTACION_SISTEMA.md
|-- rural_cli.py
|-- Makefile
|-- README.md
|-- LICENSE
`-- ruralkv.log
```

## 3. Flujo de funcionamiento

El flujo principal inicia en `src/main.c`:

1. Se crea una Arena de memoria de 1 MB.
2. Se crea una tabla hash con 1024 buckets.
3. Se abre el archivo `ruralkv.log` como Write-Ahead Log.
4. Se inicia el servidor HTTP en el puerto `8080`.
5. El servidor queda escuchando conexiones en un bucle infinito.

Cuando llega una solicitud HTTP:

1. `server.c` recibe la conexion por socket TCP.
2. Lee el request HTTP en un buffer.
3. Extrae metodo y URL usando `sscanf`.
4. Si la ruta es `/put?k=clave&v=valor`, guarda primero en el WAL y luego en la tabla hash.
5. Si la ruta es `/get?k=clave`, busca el valor en la tabla hash.
6. Responde en formato JSON basico.
7. Cierra el socket del cliente y vuelve a esperar otra conexion.

## 4. Componentes implementados

### 4.1 Arena Allocator

Archivos:

- `include/arena.h`
- `src/arena.c`

La Arena Allocator reserva un bloque grande de memoria al iniciar el programa y luego reparte porciones de ese bloque mediante un desplazamiento interno (`offset`). Esto evita llamar a `malloc` muchas veces durante la ejecucion normal del motor.

Funciones implementadas:

- `Arena* arena_init(size_t capacity)`: reserva la estructura `Arena` y su buffer interno.
- `void* arena_alloc(Arena* arena, size_t size)`: entrega memoria alineada a 8 bytes.
- `void arena_free(Arena* arena)`: libera el buffer y la estructura principal.
- `void arena_reset(Arena* arena)`: reinicia el offset a cero sin liberar el buffer.

Caracteristicas:

- Asignacion rapida en tiempo constante.
- No permite liberar objetos individuales.
- Si se llena la arena, imprime error y devuelve `NULL`.
- Es util para cargas simples donde los datos viven hasta apagar el proceso.

Limitacion importante:

Cuando se actualiza una clave existente, el valor anterior queda dentro de la Arena. No hay fuga clasica de C porque toda la Arena se libera al final, pero si se hacen muchas actualizaciones durante una misma ejecucion, el espacio de la Arena se va consumiendo.

### 4.2 Tabla Hash

Archivos:

- `include/hash.h`
- `src/hash.c`

La tabla hash es el indice principal en memoria. Guarda claves y valores como cadenas (`char*`) y resuelve colisiones mediante listas enlazadas dentro de cada bucket.

Estructuras principales:

- `HashEntry`: representa una entrada con `key`, `value` y puntero `next`.
- `HashTable`: contiene el arreglo de buckets, el tamano de la tabla y la referencia a la Arena.

Funciones implementadas:

- `HashTable* hash_create(Arena* arena, size_t size)`: crea la tabla y reserva buckets desde la Arena.
- `bool hash_put(HashTable* ht, const char* key, const char* value)`: inserta o actualiza una clave.
- `const char* hash_get(HashTable* ht, const char* key)`: busca el valor asociado a una clave.

Algoritmo usado:

- Se usa `djb2`, un algoritmo hash simple y rapido para cadenas.
- El indice final se calcula como `hash_djb2(key) % ht->size`.

Comportamiento:

- Si la clave no existe, se crea una nueva entrada.
- Si la clave ya existe, se reemplaza el puntero del valor por una nueva copia en la Arena.
- Las claves y valores se copian con una funcion interna similar a `strdup`, pero usando `arena_alloc`.

### 4.3 Write-Ahead Log

Archivos:

- `include/wal.h`
- `src/wal.c`

El WAL es un archivo append-only que registra operaciones antes de aplicarlas en memoria. En este proyecto se usa para guardar cada operacion `PUT` en `ruralkv.log`.

Funciones implementadas:

- `Wal* wal_init(const char* filepath)`: abre o crea el archivo de log en modo append binario (`ab+`).
- `bool wal_append_put(Wal* wal, const char* key, const char* value)`: escribe una linea con formato `PUT clave | valor`.
- `void wal_close(Wal* wal)`: cierra el archivo y libera la estructura.

Formato actual del log:

```text
PUT clave | valor
```

Ejemplo:

```text
PUT DNI_01 | Gripe_Alta
```

Garantia implementada:

- Cada escritura llama a `fflush`, lo que fuerza el vaciado del buffer de C hacia el sistema operativo.

Limitacion importante:

El repositorio aun no implementa recuperacion automatica desde el WAL al iniciar. Es decir, el log se escribe, pero `main.c` no lo lee para reconstruir la tabla hash despues de reiniciar el servidor.

### 4.4 Servidor HTTP nativo

Archivos:

- `include/server.h`
- `src/server.c`

El servidor abre un socket TCP y atiende peticiones HTTP muy simples. Tiene soporte condicional para Windows y sistemas tipo Unix/Linux.

Funciones principales:

- `void server_start(int port, HashTable* db, Wal* wal)`: inicia el servidor en el puerto indicado.
- `static void handle_client(...)`: atiende una conexion individual.

Rutas implementadas:

# Especificación de Endpoints - API RuralKV v0.1

Este documento detalla los endpoints HTTP nativos expuestos por el servidor en C para la gestión del motor Key-Value.

## Estructura de Rutas

### 1. Guardar o Actualizar Dato
* **Ruta:** `/put`
* **Método:** `GET` (vía URL query params)
* **Parámetros:** * `k`: Clave única (String, decodificado por URL)
  * `v`: Valor a almacenar (String, decodificado por URL)
* **Descripción:** Registra de manera persistente en el archivo WAL y actualiza la caché en la memoria RAM.

### 2. Obtener Dato
* **Ruta:** `/get`
* **Parámetros:** `k` (Clave a buscar)
* **Respuesta exitosa:** `200 OK` con JSON `{"llave": "X", "valor": "Y"}`

### 3. Eliminar Dato
* **Ruta:** `/del`
* **Parámetros:** `k` (Clave a remover)
* **Descripción:** Elimina la entrada de la memoria RAM y registra la instrucción de borrado de forma síncrona en el WAL.

### 4. Tiempo de Vida (Evicción y TTL)
* **Ruta:** `/expire` -> Asigna tiempo de expiración en segundos (`t`).
* **Ruta:** `/ttl` -> Consulta los segundos restantes de una clave (`-1` si es permanente).
Caracteristicas del servidor:

- Usa CORS abierto con `Access-Control-Allow-Origin: *`.
- Responde con `Content-Type: application/json`.
- Atiende una conexion a la vez.
- Cierra el socket despues de responder.

Limitaciones actuales:

- No hay parseo HTTP completo.
- No hay decodificacion URL (`%20`, acentos, simbolos especiales, etc.).
- No hay escape seguro de JSON.
- No hay validacion fuerte de entradas.
- No hay autenticacion ni autorizacion.
- No hay `POST`, `DELETE`, `LIST`, `UPDATE` explicito ni endpoints administrativos.
- Aunque el proyecto compila con `-pthread`, el servidor actual no crea hilos por cliente.

### 4.5 Programa principal

Archivo:

- `src/main.c`

Define constantes del sistema:

```c
#define ARENA_1MB (1024 * 1024)
#define CANTIDAD_CASILLAS_HASH 1024
#define PORT 8080
```

Responsabilidades:

- Mostrar mensajes de arranque.
- Inicializar memoria, hash table y WAL.
- Iniciar el servidor HTTP.
- Liberar recursos si el servidor termina.

Nota:

Como `server_start` contiene un bucle infinito, las llamadas a `wal_close` y `arena_free` solo se ejecutarian si el servidor sale de su ciclo.

### 4.6 Cliente CLI en Python

Archivo:

- `rural_cli.py`

El cliente permite interactuar con el servidor como si fuera una consola tipo Redis.

Comandos implementados:

```text
PUT <llave> <valor>
GET <llave>
EXIT
```

Ejemplo:

```text
rural-kv> PUT DNI_01 Gripe_Alta
OK
rural-kv> GET DNI_01
"Gripe_Alta"
```

Funcionamiento interno:

- `PUT` construye una URL `/put?k=<llave>&v=<valor>`.
- `GET` construye una URL `/get?k=<llave>`.
- Usa `urllib.request` para llamar al servidor.
- Usa `json.loads` para leer respuestas exitosas.

Limitacion:

El CLI no codifica parametros de URL. Valores con espacios, acentos o caracteres especiales pueden fallar o llegar incompletos.

## 5. Persistencia y recuperacion

La persistencia implementada consiste en escribir operaciones `PUT` en `ruralkv.log`.

Ejemplo de ciclo:

1. El usuario ejecuta `PUT DNI_01 Gripe_Alta`.
2. El servidor escribe `PUT DNI_01 | Gripe_Alta` en `ruralkv.log`.
3. Luego actualiza la tabla hash en memoria.
4. Mientras el proceso siga vivo, `GET DNI_01` devuelve el valor.

Estado actual de recuperacion:

- El archivo WAL conserva las operaciones.
- El programa no reconstruye la base en memoria al reiniciar.
- Para que la recuperacion sea completa, falta implementar una funcion que lea `ruralkv.log` durante el arranque y reaplique cada operacion `PUT`.

## 6. Compilacion y ejecucion

### Compilar con Make

En sistemas con `make` disponible:

```bash
make
```

En Windows, el `Makefile` agrega `-lws2_32` para enlazar Winsock.

### Compilar directamente con GCC

```bash
gcc -Wall -Wextra -pthread -std=c11 -O2 -I./include src/*.c -o ruralkv.exe -lws2_32
```

En Linux/macOS normalmente no se necesita `-lws2_32`:

```bash
gcc -Wall -Wextra -pthread -std=c11 -O2 -I./include src/*.c -o ruralkv
```

### Ejecutar servidor

En Windows:

```powershell
.\ruralkv.exe
```

En Linux/macOS:

```bash
./ruralkv
```

El servidor queda disponible en:

```text
http://localhost:8080/
```

### Ejecutar cliente CLI

Con el servidor encendido en otra terminal:

```bash
python rural_cli.py
```

## 7. API HTTP actual

### Estado del servidor

```http
GET /
```

### Guardar dato

```http
GET /put?k=<clave>&v=<valor>
```

Ejemplo:

```text
http://localhost:8080/put?k=paciente_1&v=vacuna_aplicada
```

### Leer dato

```http
GET /get?k=<clave>
```

Ejemplo:

```text
http://localhost:8080/get?k=paciente_1
```

## 8. Estado de caracteristicas

| Caracteristica | Estado actual |
| --- | --- |
| Motor clave-valor en memoria | Implementado |
| Arena Allocator | Implementado |
| Hash con encadenamiento | Implementado |
| WAL append-only para `PUT` | Implementado |
| Servidor HTTP basico | Implementado |
| Cliente CLI Python | Implementado |
| Compilacion por Makefile | Implementado |
| Compatibilidad Windows/Linux en sockets | Parcialmente implementada |
| Recuperacion desde WAL al reiniciar | No implementada |
| Concurrencia multihilo real | No implementada |
| Pub/Sub | No implementado |
| Delta Sync con nube | No implementado |
| Interfaz HTMX/Alpine.js | No implementada en el repositorio |
| Autenticacion | No implementada |
| Cifrado | No implementado |
| Eliminacion de claves | No implementada |
| Listado de claves | No implementado |

## 9. Consideraciones tecnicas

### Seguridad

El sistema actual debe considerarse una API local de prueba. No deberia exponerse directamente a internet porque:

- Acepta cualquier origen CORS.
- No valida identidad del usuario.
- No sanitiza ni escapa respuestas JSON.
- No limita tamano real de solicitudes mas alla de los buffers internos.
- No maneja rutas malformadas de forma robusta.

### Memoria

La memoria se concentra en una Arena de 1 MB. Este diseno simplifica la gestion y reduce fragmentacion, pero tambien significa que:

- No se liberan claves o valores individualmente.
- Muchas escrituras o actualizaciones pueden llenar la Arena.
- Al llenarse, nuevas inserciones pueden fallar.

### Red

El servidor actual es secuencial:

- Acepta una conexion.
- La atiende.
- La cierra.
- Luego acepta la siguiente.

Esto es suficiente para una demostracion simple, pero no para alta concurrencia real.

### Persistencia

El WAL registra operaciones, pero no hay replay automatico. Por eso la persistencia todavia esta a medio camino: el historial queda en disco, pero el estado en memoria se pierde al reiniciar hasta que se implemente la recuperacion.

## 10. Posibles mejoras futuras

Mejoras recomendadas para completar el sistema:

- Implementar `wal_replay` para reconstruir la tabla hash desde `ruralkv.log`.
- Cambiar `/put` a metodo `POST` y recibir JSON.
- Codificar y decodificar correctamente parametros URL.
- Escapar cadenas JSON antes de responder.
- Agregar endpoint `DELETE`.
- Agregar endpoint `LIST` o iterador de claves.
- Agregar locks o modelo multihilo si se atienden clientes concurrentes.
- Separar parser HTTP de la logica de base de datos.
- Agregar pruebas unitarias para Arena, Hash y WAL.
- Agregar pruebas de integracion para la API HTTP.
- Definir un formato WAL mas robusto con longitud de campos o checksum.
- Implementar autenticacion si se usa con datos sensibles.

## 11. Resumen

RuralKV ya tiene el nucleo minimo de una base clave-valor local: reserva memoria con Arena, indexa datos con una tabla hash, registra escrituras en un WAL y expone operaciones basicas mediante HTTP y CLI.

El sistema es ideal como prototipo educativo o base inicial para una solucion offline-first. Para convertirlo en un motor resistente en produccion, el siguiente paso mas importante es implementar recuperacion desde WAL, mejorar el parser de entrada y agregar controles de seguridad.
