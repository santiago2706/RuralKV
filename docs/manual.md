# Manual de Uso: Motor RuralKV Engine

Este manual explica como compilar, ejecutar y utilizar el motor RuralKV, ademas de describir las rutas HTTP disponibles y el uso del cliente CLI.

## 1. Requisitos

- GCC compatible con C11.
- Windows o Linux con soporte de sockets.
- Python 3 para ejecutar el cliente `rural_cli.py`.
- El repositorio debe tener los archivos `Makefile`, `src/*.c`, `include/*.h` y `rural_cli.py`.

## 2. Compilacion

### 2.1 Usando `Makefile`

En la raiz del proyecto, ejecute:

```bash
make
```

Esto genera el ejecutable `ruralkv` en Linux o `ruralkv.exe` en Windows.

### 2.2 Compilacion directa con GCC

#### En Windows (PowerShell / CMD)

```bash
gcc -Wall -Wextra -pthread -std=c11 -O2 -I./include src/arena.c src/hash.c src/main.c src/server.c src/wal.c -o ruralkv.exe -lws2_32
```

#### En Linux / macOS

```bash
gcc -Wall -Wextra -pthread -std=c11 -O2 -I./include src/*.c -o ruralkv
```

## 3. Ejecucion del servidor

Una vez compilado, inicie el servidor con:

```bash
./ruralkv
```

o en Windows:

```bash
ruralkv.exe
```

El servidor arranca en el puerto `8080` y permanece escuchando en un bucle infinito.
Antes de aceptar conexiones, reconstruye la tabla hash desde `ruralkv.log` mediante replay del WAL.

## 4. Uso del cliente CLI

Ejecute el cliente Python desde la raiz del proyecto:

```bash
python rural_cli.py
```

### Comandos disponibles

- `PUT <llave> <valor>`: guarda o actualiza un valor.
- `GET <llave>`: consulta el valor asociado.
- `EXIT`: cierra el cliente.

Ejemplo:

```text
rural-kv> PUT DNI_01 Gripe_Alta
OK
rural-kv> GET DNI_01
"Gripe_Alta"
```

## 5. API HTTP disponible

### 5.1 Guardar o actualizar

- Ruta: `/put?k=<clave>&v=<valor>`
- Metodo: GET
- Ejemplo:

```text
http://localhost:8080/put?k=DNI_01&v=Gripe_Alta
```

### 5.2 Recuperar un valor

- Ruta: `/get?k=<clave>`
- Metodo: GET
- Ejemplo:

```text
http://localhost:8080/get?k=DNI_01
```

### 5.3 Eliminar una clave

- Ruta: `/del?k=<clave>`
- Metodo: GET
- Ejemplo:

```text
http://localhost:8080/del?k=DNI_01
```

### 5.4 Verificar existencia

- Ruta: `/exists?k=<clave>`
- Metodo: GET
- Ejemplo:

```text
http://localhost:8080/exists?k=DNI_01
```

### 5.5 Listar claves activas

- Ruta: `/keys`
- Metodo: GET
- Ejemplo:

```text
http://localhost:8080/keys
```

### 5.6 Asignar expiracion (TTL)

- Ruta: `/expire?k=<clave>&t=<segundos>`
- Metodo: GET
- Ejemplo:

```text
http://localhost:8080/expire?k=DNI_01&t=60
```

### 5.7 Consultar tiempo restante

- Ruta: `/ttl?k=<clave>`
- Metodo: GET
- Ejemplo:

```text
http://localhost:8080/ttl?k=DNI_01
```

### 5.8 Comprobar que el servidor responde

- Ruta: `/ping`
- Metodo: GET
- Ejemplo:

```text
http://localhost:8080/ping
```

### 5.9 Informacion de estado

- Ruta: `/info`
- Metodo: GET
- Ejemplo:

```text
http://localhost:8080/info
```

## 6. Formato del WAL

El archivo `ruralkv.log` registra operaciones en este formato:

- `PUT <clave> | <valor>`
- `DEL <clave>`
- `EXPIRE <clave> <segundos>`

El WAL se actualiza antes de que el servidor aplique la modificacion en memoria. Durante el arranque, el servidor reinterpreta ese registro y restaura la HashTable.

## 7. Notas importantes

- El servidor restaura automaticamente el estado desde `ruralkv.log` al arrancar.
- El servidor atiende un cliente a la vez.
- No existe autenticacion ni control de acceso.
- El parser HTTP es minimalista; los parametros deben enviarse en la forma exacta esperada.

## 8. Desarrollo y mantenimiento

### Archivos clave

- `src/main.c`: inicializacion de memoria, tabla hash, WAL y servidor.
- `src/arena.c`: implementa el gestor de memoria.
- `src/hash.c`: implementa la logica de almacenamiento, eliminacion y TTL.
- `src/server.c`: atiende las peticiones HTTP y orquesta los endpoints.
- `src/wal.c`: escribe el log de operaciones y recupera el estado al iniciar.
- `include/*.h`: interfaces de los modulos.
- `rural_cli.py`: cliente de prueba e interaccion basica.

### Recomendaciones para avanzar

- Añadir pruebas unitarias para `hash.c`, `wal.c` y `server.c`.
- Mejorar el parseo HTTP y soportar metodos `POST`.
- Agregar documentacion de casos de prueba y despliegue.
