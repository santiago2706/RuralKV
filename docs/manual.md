# Manual de Uso: Motor RuralKV Engine

Este manual explica cómo compilar, ejecutar y utilizar el motor RuralKV, además de describir las rutas HTTP disponibles y el uso del cliente CLI.

## 1. Requisitos

- GCC compatible con C11.
- Windows o Linux con soporte de sockets.
- Python 3 para ejecutar el cliente `rural_cli.py`.
- El repositorio debe tener los archivos `Makefile`, `src/*.c`, `include/*.h` y `rural_cli.py`.

## 2. Compilación

### 2.1 Usando `Makefile`

En la raíz del proyecto, ejecute:

```bash
make
```

Esto genera el ejecutable `ruralkv` en Linux o `ruralkv.exe` en Windows.

### 2.2 Compilación directa con GCC

#### En Windows (PowerShell / CMD)

```bash
gcc -Wall -Wextra -pthread -std=c11 -O2 -I./include src/arena.c src/hash.c src/main.c src/server.c src/wal.c -o ruralkv.exe -lws2_32
```

#### En Linux / macOS

```bash
gcc -Wall -Wextra -pthread -std=c11 -O2 -I./include src/*.c -o ruralkv
```

## 3. Ejecución del servidor

Una vez compilado, inicie el servidor con:

```bash
./ruralkv
```

o en Windows:

```bash
ruralkv.exe
```

El servidor arranca en el puerto `8080` y permanece escuchando en un bucle infinito.

## 4. Uso del cliente CLI

Ejecute el cliente Python desde la raíz del proyecto:

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
- Método: GET
- Ejemplo:

```text
http://localhost:8080/put?k=DNI_01&v=Gripe_Alta
```

### 5.2 Recuperar un valor

- Ruta: `/get?k=<clave>`
- Método: GET
- Ejemplo:

```text
http://localhost:8080/get?k=DNI_01
```

### 5.3 Eliminar una clave

- Ruta: `/del?k=<clave>`
- Método: GET
- Ejemplo:

```text
http://localhost:8080/del?k=DNI_01
```

### 5.4 Verificar existencia

- Ruta: `/exists?k=<clave>`
- Método: GET
- Ejemplo:

```text
http://localhost:8080/exists?k=DNI_01
```

### 5.5 Listar claves activas

- Ruta: `/keys`
- Método: GET
- Ejemplo:

```text
http://localhost:8080/keys
```

### 5.6 Asignar expiración (TTL)

- Ruta: `/expire?k=<clave>&t=<segundos>`
- Método: GET
- Ejemplo:

```text
http://localhost:8080/expire?k=DNI_01&t=60
```

### 5.7 Consultar tiempo restante

- Ruta: `/ttl?k=<clave>`
- Método: GET
- Ejemplo:

```text
http://localhost:8080/ttl?k=DNI_01
```

### 5.8 Comprobar que el servidor responde

- Ruta: `/ping`
- Método: GET
- Ejemplo:

```text
http://localhost:8080/ping
```

### 5.9 Información de estado

- Ruta: `/info`
- Método: GET
- Ejemplo:

```text
http://localhost:8080/info
```

## 6. Formato del WAL

El archivo `ruralkv.log` registra operaciones en este formato:

- `PUT <clave> | <valor>`
- `DEL <clave>`
- `EXPIRE <clave> <segundos>`

El WAL se actualiza antes de que el servidor aplique la modificación en memoria.

## 7. Notas importantes

- El servidor no restaura automáticamente el estado desde el WAL al arrancar.
- El servidor atiende un cliente a la vez.
- No existe autenticación ni control de acceso.
- El parser HTTP es minimalista; los parámetros deben enviarse en la forma exacta esperada.

## 8. Desarrollo y mantenimiento

### Archivos clave

- `src/main.c`: inicialización de memoria, tabla hash, WAL y servidor.
- `src/arena.c`: implementa el gestor de memoria.
- `src/hash.c`: implementa la lógica de almacenamiento, eliminación y TTL.
- `src/server.c`: atiende las peticiones HTTP y orquesta los endpoints.
- `src/wal.c`: escribe el log de operaciones.
- `include/*.h`: interfaces de los módulos.
- `rural_cli.py`: cliente de prueba e interacción básica.

### Recomendaciones para avanzar

- Implementar lectura de `ruralkv.log` al iniciar el servidor.
- Añadir pruebas unitarias para `hash.c`, `wal.c` y `server.c`.
- Mejorar el parseo HTTP y soportar métodos `POST`.
- Agregar documentación de casos de prueba y despliegue.
