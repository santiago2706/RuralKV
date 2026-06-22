# Documentacion del Sistema RuralKV

## 1. Objetivo

Este documento describe la arquitectura, el comportamiento y el estado actual del motor RuralKV. Esta orientado a quienes desarrollan, mantienen o integran el proyecto, entregando informacion precisa sobre los componentes existentes, los endpoints disponibles y las limitaciones actuales.

## 2. Vision general

RuralKV es un motor de almacenamiento clave-valor escrito en C, concebido para operar en entornos con recursos limitados. La implementacion actual se enfoca en:

- almacenamiento en memoria de pares `clave -> valor`,
- persistencia mediante un Write-Ahead Log (WAL),
- snapshots binarios para acelerar la recuperacion,
- servidor HTTP nativo con rutas basicas,
- gestion de TTL por clave,
- un cliente CLI en Python para pruebas funcionales.

El proyecto esta pensado como la base tecnica de una solucion tipo ImmortalData para zonas rurales con conectividad intermitente.

## 3. Alcance actual

### Funcionalidades implementadas

- `HashTable` en memoria con colisiones por encadenamiento.
- `Arena Allocator` para asignaciones rapidas y predecibles.
- Registro append-only de operaciones (`ruralkv.log`).
- Snapshots binarios manuales o temporales (`snapshot.bin`).
- Recuperacion automatica del estado al iniciar mediante snapshot + replay del WAL.
- Endpoints HTTP para CRUD basico y TTL.
- Cliente Python interactivo (`rural_cli.py`).
- Compilacion multiplataforma con `Makefile`.

### Funcionalidades pendientes

- Manejo de multiples clientes simultaneos.
- Autenticacion y control de acceso.
- Parser HTTP completo y robusto.
- Pruebas unitarias e integracion automatizada.
- Interfaz web moderna o frontend ligero.

## 4. Arquitectura de componentes

### 4.1 Arena Allocator

- Archivo: `src/arena.c`
- Interfaz: `include/arena.h`

La Arena administra un bloque continuo de memoria y atiende solicitudes de tamanio variable mediante un desplazamiento interno. Esto minimiza llamadas a `malloc` y mantiene una gestion de memoria simple.

Principales caracteristicas:

- asignacion en tiempo constante,
- alineacion de 8 bytes,
- liberacion completa con `arena_free()`,
- reseteo de uso con `arena_reset()`.

### 4.2 Tabla Hash

- Archivo: `src/hash.c`
- Interfaz: `include/hash.h`

La `HashTable` es la estructura principal de datos en RAM. Cada `HashEntry` almacena:

- `key`,
- `value`,
- `expire_at` (TTL),
- puntero `next` para manejar colisiones.

Funciones clave:

- `hash_create()`
- `hash_put()`
- `hash_get()`
- `hash_delete()`
- `hash_exists()`
- `hash_set_expire()`
- `hash_ttl()`

### 4.3 Write-Ahead Log (WAL)

- Archivo: `src/wal.c`
- Interfaz: `include/wal.h`

El WAL registra las operaciones antes de aplicarlas en memoria. Actualmente soporta tres tipos de entradas:

- `PUT <clave> | <valor>`
- `DEL <clave>`
- `EXPIRE <clave> <segundos>`

Cada insercion se sincroniza con `fflush()` para mayor durabilidad.

Al iniciar el servidor, `wal_replay()` reabre `ruralkv.log`, reconstruye la `HashTable` y reporta cuantas operaciones fueron recuperadas.

### 4.4 Snapshot binario

- Archivo: `src/snapshot.c`
- Interfaz: `include/snapshot.h`

El snapshot guarda una copia completa del estado activo de la `HashTable` en `snapshot.bin` para acelerar el arranque. El formato binario incluye:

- total de entradas,
- longitud de clave,
- bytes de clave,
- longitud de valor,
- bytes de valor,
- `expire_at`.

Al cargar, `snapshot_load()` reconstruye las entradas activas y descarta las que ya expiraron.

### 4.5 Servidor HTTP nativo

- Archivo: `src/server.c`
- Interfaz: `include/server.h`

El servidor escucha en el puerto `8080` y atiende peticiones HTTP simples. La implementacion soporta Windows y Unix mediante condicionales de compilacion.

Rutas actualmente soportadas:

- `/put?k=<clave>&v=<valor>`
- `/get?k=<clave>`
- `/del?k=<clave>`
- `/exists?k=<clave>`
- `/keys`
- `/expire?k=<clave>&t=<segundos>`
- `/ttl?k=<clave>`
- `/ping`
- `/snapshot`
- `/info`

### 4.6 Cliente CLI en Python

- Archivo: `rural_cli.py`

Proporciona un acceso de consola estilo Redis para pruebas manuales contra el servidor HTTP local.

Comandos soportados:

- `PUT <llave> <valor>`
- `GET <llave>`
- `DEL <llave>`
- `EXISTS <llave>`
- `KEYS`
- `EXPIRE <llave> <segundos>`
- `TTL <llave>`
- `PING`
- `INFO`
- `CLEAR`
- `HELP`
- `MEM`
- `EXIT`

## 5. Flujo de datos

1. `main.c` inicializa el `Arena`, la `HashTable` y el `WAL`.
2. `snapshot_load()` reconstruye primero el estado desde `snapshot.bin`.
3. `wal_replay()` aplica luego las operaciones mas recientes desde `ruralkv.log`.
4. El servidor inicia en puerto `8080`.
5. Cada peticion entrante se parsea de forma simplificada en `server.c`.
6. En `/put`, la operacion se escribe en WAL y luego en la tabla hash.
7. En `/get`, se consulta la tabla hash y se devuelve el valor activo.
8. En `/expire`, se configura TTL y se elimina la entrada cuando expira.

## 6. Endpoints y ejemplos

### 6.1 Guardar o actualizar

`GET /put?k=<clave>&v=<valor>`

Ejemplo:

```text
http://localhost:8080/put?k=usuario1&v=vacuna_aplicada
```

### 6.2 Leer valor

`GET /get?k=<clave>`

Ejemplo:

```text
http://localhost:8080/get?k=usuario1
```

### 6.3 Eliminar valor

`GET /del?k=<clave>`

Ejemplo:

```text
http://localhost:8080/del?k=usuario1
```

### 6.4 Verificar existencia

`GET /exists?k=<clave>`

Ejemplo:

```text
http://localhost:8080/exists?k=usuario1
```

### 6.5 Listar claves activas

`GET /keys`

Ejemplo:

```text
http://localhost:8080/keys
```

### 6.6 Asignar expiracion (TTL)

`GET /expire?k=<clave>&t=<segundos>`

Ejemplo:

```text
http://localhost:8080/expire?k=usuario1&t=120
```

### 6.7 Consultar TTL

`GET /ttl?k=<clave>`

Ejemplo:

```text
http://localhost:8080/ttl?k=usuario1
```

### 6.8 Comprobar salud del servidor

`GET /ping`

Ejemplo:

```text
http://localhost:8080/ping
```

### 6.9 Generar snapshot manual

`GET /snapshot`

Ejemplo:

```text
http://localhost:8080/snapshot
```

### 6.10 Informacion del motor

`GET /info`

Ejemplo:

```text
http://localhost:8080/info
```

## 7. Estado actual y limitaciones

### Implementado

- Almacenamiento en memoria clave-valor.
- Persistencia append-only en `ruralkv.log`.
- Persistencia por snapshot binario en `snapshot.bin`.
- Recuperacion automatica del estado al iniciar mediante snapshot + WAL replay.
- Endpoints HTTP CRUD basicos.
- Cliente CLI Python.
- Soporte de TTL y vencimiento de entradas.

### Falta por implementar

- Manejo de multiples clientes simultaneos.
- Soporte completo de HTTP/1.1.
- Autenticacion y validacion de entrada.
- Pruebas automatizadas.
- Compactacion de datos en memoria.

## 8. Proceso de construccion del proyecto

1. Se definio la necesidad de un motor ligero de clave-valor para entornos desconectados.
2. Se diseno una arquitectura modular con memoria, indice, registro y servidor.
3. Se implemento el `Arena Allocator` como primera capa de memoria.
4. Se desarrollo la `HashTable` para indexacion y busqueda.
5. Se anadio el `WAL` para asegurar durabilidad de escritura.
6. Se creo el servidor HTTP y se definieron los endpoints de prueba.
7. Se construyo el cliente CLI para validar flujo de uso.
8. Se amplio el cliente CLI con comandos de administracion y diagnostico: `DEL`, `EXISTS`, `KEYS`, `EXPIRE`, `TTL`, `PING`, `INFO`, `CLEAR`, `HELP` y `MEM`.
9. Se creo un entorno virtual local `.venv/` para aislar dependencias de Python durante el desarrollo.
10. Se actualizo `.gitignore` para excluir `.venv/` y artefactos generados como ejecutables, objetos y logs.
11. Se dejo documentado el estado actual, las posibles mejoras y los riesgos de exposicion.

## 9. Recomendaciones tecnicas

- Usar el WAL como respaldo y validar periodicamente que el replay siga funcionando.
- Ejecutar el servidor en un entorno cerrado.
- Evitar actualizar la misma clave demasiadas veces en una sola sesion para no agotar la Arena.
- Priorizar la adicion de pruebas unitarias antes de ampliar la API.

## 10. Estructura de archivos relevante

- `.gitignore`
- `src/main.c`
- `src/arena.c`
- `src/hash.c`
- `src/server.c`
- `src/snapshot.c`
- `src/wal.c`
- `include/arena.h`
- `include/hash.h`
- `include/server.h`
- `include/snapshot.h`
- `include/wal.h`
- `rural_cli.py`
- `requirements.txt`
- `Makefile`
- `docs/manual.md`
- `docs/documentacion-sistema.md`

### Entorno de desarrollo Python

El entorno virtual `.venv/` se usa solo de forma local para ejecutar herramientas y dependencias de Python, por ejemplo el cliente `rural_cli.py`. No forma parte del codigo fuente versionado y queda excluido mediante `.gitignore`.

La lista de dependencias compartibles del proyecto debe mantenerse en `requirements.txt`.

### Red

El servidor actual es secuencial:

- Acepta una conexion.
- La atiende.
- La cierra.
- Luego acepta la siguiente.

Esto es suficiente para una demostracion simple, pero no para alta concurrencia real.

### Persistencia

El WAL registra operaciones y el servidor las reinterpreta al iniciar para reconstruir la tabla hash en memoria. Ademas, el snapshot permite restaurar una copia compacta del estado antes de aplicar el WAL, reduciendo el tiempo de arranque.

## 11. Posibles mejoras futuras

Mejoras recomendadas para completar el sistema:

- Mejorar el parser HTTP y soportar metodos `POST`.
- Codificar y decodificar correctamente parametros URL.
- Escapar cadenas JSON antes de responder.
- Agregar endpoint `DELETE`.
- Agregar endpoint `LIST` o iterador de claves.
- Agregar locks o modelo multihilo si se atienden clientes concurrentes.
- Separar parser HTTP de la logica de base de datos.
- Agregar pruebas unitarias para Arena, Hash, WAL y Snapshot.
- Agregar pruebas de integracion para la API HTTP.
- Definir un formato WAL mas robusto con longitud de campos o checksum.
- Implementar autenticacion si se usa con datos sensibles.

## 12. Resumen

RuralKV ya tiene el nucleo minimo de una base clave-valor local: reserva memoria con Arena, indexa datos con una tabla hash, registra escrituras en un WAL, guarda snapshots compactos, recupera el estado al arrancar y expone operaciones basicas mediante HTTP y CLI.

El sistema es ideal como prototipo educativo o base inicial para una solucion offline-first. Para convertirlo en un motor resistente en produccion, el siguiente paso importante es mejorar el parser de entrada, agregar controles de seguridad y ampliar las pruebas automatizadas.
