# 🌱 RuralKV

**Motor Clave-Valor Ultra-Ligero Offline-First para Zonas Rurales de Perú**

Proyecto desarrollado para la Feria de Ingeniería de Software - UNI.

---

## 🚀 Descripción

RuralKV es un motor de base de datos clave-valor desarrollado en C (C11), diseñado para funcionar en entornos con conectividad limitada y hardware de bajos recursos, como zonas rurales del Perú.

El sistema permite almacenar, consultar y sincronizar datos de manera eficiente, incluso sin conexión a internet.

---

## 🎯 Objetivos

* Funcionar 100% offline
* Minimizar uso de memoria (< 8MB)
* Reducir consumo de datos hasta 95% (Delta Sync)
* Operar en dispositivos como Raspberry Pi o ESP32
* Implementar estructuras de bajo nivel en C

---

## ⚙️ Características principales

* 🧠 Arena Allocator (gestión eficiente de memoria)
* 🗂️ Tabla Hash (almacenamiento clave-valor)
* 💾 Persistencia binaria (SAVE / LOAD)
* ⏱️ TTL (expiración de datos)
* 👥 Concurrencia (pthreads)
* 📢 Pub/Sub (mensajería en tiempo real)
* 🔄 Delta Sync (sincronización eficiente)
* 🌐 Servidor HTTP mínimo

---

## 🧱 Arquitectura

```text
Usuario / Cliente
       ↓
   REPL / HTTP
       ↓
   Tabla Hash
       ↓
 Arena Allocator
       ↓
 Persistencia (archivo binario)
       ↓
 Concurrencia (threads)
       ↓
 Pub/Sub + Delta Sync
```

---

## 🛠️ Tecnologías

* Lenguaje: C (C11)
* POSIX Sockets
* Pthreads
* Makefile
* Valgrind / Address Sanitizer
* gcov (testing)

---

## 📦 Estructura del proyecto

```text
RuralKV/
 ├── src/
 ├── include/
 ├── tests/
 ├── docs/
 ├── main.c
 ├── Makefile
 └── README.md
```

---

## 🚧 Estado del proyecto

* [x] Estructura base
* [ ] Arena Allocator
* [ ] Tabla Hash
* [ ] REPL
* [ ] Persistencia
* [ ] TTL
* [ ] Concurrencia
* [ ] Pub/Sub
* [ ] Delta Sync
* [ ] HTTP Server

---

## ▶️ Cómo ejecutar

```bash
make
./ruralkv
```

---

## 🧪 Testing

```bash
valgrind ./ruralkv
```

---

## 🌍 Impacto

RuralKV busca mejorar la gestión de datos en:

* Agricultura rural 🌾
* Salud comunitaria 🏥
* IoT en zonas sin conectividad 📡

---

## 👨‍💻 Equipo

Proyecto desarrollado por estudiantes de Ingeniería de Software - UNI.

---

## 📄 Licencia

MIT License
