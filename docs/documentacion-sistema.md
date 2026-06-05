# Documentación del Sistema RuralKV

## 1. Objetivo

Este documento describe la arquitectura, el comportamiento y el estado actual del motor RuralKV. Está orientado a quienes desarrollan, mantienen o integran el proyecto, entregando información precisa sobre los componentes existentes, los endpoints disponibles y las limitaciones actuales.

## 2. Visión general

RuralKV es un motor de almacenamiento clave-valor escrito en C, concebido para operar en entornos con recursos limitados. La implementación actual se enfoca en:

- almacenamiento en memoria de pares `clave -> valor`,
- persistencia mediante un Write-Ahead Log (WAL),
- servidor HTTP nativo con rutas básicas,
- gestión de TTL por clave,
- un cliente CLI en Python para pruebas funcionales.

El proyecto está pensado como la base técnica de una solución tipo ImmortalData para zonas rurales con conectividad intermitente.

## 3. Alcance actual

### Funcionalidades implementadas

- `HashTable` en memoria con colisiones por encadenamiento.
- `Arena Allocator` para asignaciones rápidas y predecibles.
- Registro append-only de operaciones (`ruralkv.log`).
- Endpoints HTTP para CRUD básico y TTL.
- Cliente Python interactivo (`rural_cli.py`).
- Compilación multiplataforma con `Makefile`.

### Funcionalidades pendientes

- Recuperación automática desde WAL al iniciar.
- Manejo de múltiples clientes simultáneos.
- Autenticación y control de acceso.
- Parser HTTP completo y robusto.
- Pruebas unitarias e integración automatizada.
- Interfaz web moderna o frontend ligero.

## 4. Arquitectura de componentes

### 4.1 Arena Allocator

- Archivo: `src/arena.c`
- Interfaz: `include/arena.h`

La Arena administra un bloque continuo de memoria y atiende solicitudes de tamaño variable mediante un desplazamiento interno. Esto minimiza llamadas a `malloc` y mantiene una gestión de memoria simple.

Principales características:

- asignación en tiempo constante,
- alineación de 8 bytes,
- liberación completa con `arena_free()`,
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

Cada inserción se sincroniza con `fflush()` para mayor durabilidad.

### 4.4 Servidor HTTP nativo

- Archivo: `src/server.c`
- Interfaz: `include/server.h`

El servidor escucha en el puerto `8080` y atiende peticiones HTTP simples. La implementación soporta Windows y Unix mediante condicionales de compilación.

Rutas actualmente soportadas:

- `/put?k=<clave>&v=<valor>`
- `/get?k=<clave>`
- `/del?k=<clave>`
- `/exists?k=<clave>`
- `/keys`
- `/expire?k=<clave>&t=<segundos>`
- `/ttl?k=<clave>`
- `/ping`
- `/info`

### 4.5 Cliente CLI en Python

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

1. `main.c` inicializa el `Arena`, la `HashTable` y el `Wal`.
2. El servidor inicia en puerto `8080`.
3. Cada petición entrante se parsea de forma simplificada en `server.c`.
4. En `/put`, la operación se escribe en WAL y luego en la tabla hash.
5. En `/get`, se consulta la tabla hash y se devuelve el valor activo.
6. En `/expire`, se configura TTL y se elimina la entrada cuando expira.

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

### 6.6 Asignar expiración (TTL)

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

### 6.9 Información del motor

`GET /info`

Ejemplo:

```text
http://localhost:8080/info
```

## 7. Estado actual y limitaciones

### Implementado

- Almacenamiento en memoria clave-valor.
- Persistencia append-only en `ruralkv.log`.
- Endpoints HTTP CRUD básicos.
- Cliente CLI Python.
- Soporte de TTL y vencimiento de entradas.

### Falta por implementar

- Recuperación del estado desde WAL al iniciar.
- Manejo de múltiples clientes simultáneos.
- Soporte completo de HTTP/1.1.
- Autenticación y validación de entrada.
- Pruebas automatizadas.
- Compactación de datos en memoria.

## 8. Proceso de construcción del proyecto

1. Se definió la necesidad de un motor ligero de clave-valor para entornos desconectados.
2. Se diseñó una arquitectura modular con memoria, índice, registro y servidor.
3. Se implementó el `Arena Allocator` como primera capa de memoria.
4. Se desarrolló la `HashTable` para indexación y búsqueda.
5. Se añadió el `WAL` para asegurar durabilidad de escritura.
6. Se creó el servidor HTTP y se definieron los endpoints de prueba.
7. Se construyó el cliente CLI para validar flujo de uso.
8. Se amplió el cliente CLI con comandos de administración y diagnóstico: `DEL`, `EXISTS`, `KEYS`, `EXPIRE`, `TTL`, `PING`, `INFO`, `CLEAR`, `HELP` y `MEM`.
9. Se creó un entorno virtual local `.venv/` para aislar dependencias de Python durante el desarrollo.
10. Se actualizó `.gitignore` para excluir `.venv/` y artefactos generados como ejecutables, objetos y logs.
11. Se dejó documentado el estado actual, las posibles mejoras y los riesgos de exposición.

## 9. Recomendaciones técnicas

- Usar el WAL como respaldo, pero no confiar en él hasta agregar recuperación al arranque.
- Ejecutar el servidor en un entorno cerrado.
- Evitar actualizar la misma clave demasiadas veces en una sola sesión para no agotar la Arena.
- Priorizar la adición de pruebas unitarias antes de ampliar la API.

## 10. Estructura de archivos relevante

- `.gitignore`
- `src/main.c`
- `src/arena.c`
- `src/hash.c`
- `src/server.c`
- `src/wal.c`
- `include/arena.h`
- `include/hash.h`
- `include/server.h`
- `include/wal.h`
- `rural_cli.py`
- `requirements.txt`
- `Makefile`
- `docs/MANUAL.md`
- `docs/DOCUMENTACION_SISTEMA.md`

### Entorno de desarrollo Python

El entorno virtual `.venv/` se usa solo de forma local para ejecutar herramientas y dependencias de Python, por ejemplo el cliente `rural_cli.py`. No forma parte del código fuente versionado y queda excluido mediante `.gitignore`.

La lista de dependencias compartibles del proyecto debe mantenerse en `requirements.txt`.

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
