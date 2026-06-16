#  RuralKV

### Software Edge-offline y online  de Registros de Salud Inquebrantable para Zonas Rurales

> **El motor clandestino:** ImmortalData funciona gracias a RuralKV, nuestro motor de almacenamiento Clave-Valor ultraligero construido desde cero en C (C11)

---
#Autores 
* **Luis Angel Santiago Ayala
* **Alan Josue Monje MAron
* **Daniel Efren Puertas FLores
* **Aldair Samir Piñas Camarena
## 1 La Problemática

En las postas médicas y centros comunales de las zonas rurales del Perú, la gestión de datos vitales (historias clínicas, vacunas, registro de nacimientos, alertas epidemiológicas) se enfrenta a una realidad tecnológica hostil:

*   **Cero Internet (o intermitente):** Las soluciones modernas "Cloud-first" (Dependientes de la nube) quedan completamente inutilizadas.
*   **Apagones Constantes:** Los cortes de luz imprevistos generan pérdidas de datos diarios.
*   **Alternativas Precarias:** Los médicos y enfermeras recurren al papel y lápiz (que se deteriora o se pierde) o a usar un Excel offline en una laptop. Sin embargo, un Excel no permite que múltiples enfermeras cooperen al mismo tiempo y no tiene un protocolo automático para subir los datos sin colisiones cuando el internet por fin regresa.

##  2. La Solución: "La Nube Local de Cajón"

**ImmortalData** transforma esta debilidad en su mayor ventaja. Instalando un servidor económico (como una Raspberry Pi o una laptop vieja) en la posta médica, se levanta una **Intranet inquebrantable**. 

Las enfermeras usan una interfaz gráfica veloz en sus celulares o tablets para registrar pacientes y coordinar turnos en **tiempo real**, aunque el pueblo entero esté desconectado del resto del mundo.

**¿El secreto que hace esto posible?** Bajo la interfaz gráfica no usamos bases comerciales gigantes. Hemos desarrollado **RuralKV**, el motor *Edge-Cache* *key-value* que se encarga de alojar la red entera usando recursos mínimos, soportando concurrencia local y garantizando la supervivencia de la data.

##  3. ImmortalData vs Herramientas Tradicionales

| Característica | ImmortalData (RuralKV Engine) |  Excel Offline |  WhatsApp (Mensajes) |
| :--- | :--- | :--- | :--- |
| **Colaboración Sin Internet** | **SÍ.** Red Local que permite múltiples enfermeras editando y viendo datos en vivo. | **NO.** Se aíslan copias locales generando versiones conflictivas. | **NO.** Requiere viajar a California y regresar al pueblo para enviar un mensaje. |
| **Supervivencia a Apagones** | **EXTREMA.** Nuestro *Write-Ahead Log* recupera el estado intacto localmente. | **BAJA.** Frecuente corrupción de archivos guardados a medias. | - |
| **Retorno de Conexión a Internet** | **AUTOMÁTICA (Delta Sync).** Comprime y envía solo los *kilobytes* estrictamente nuevos a Lima. Sin colisiones. | **MANUAL.** Obliga a subir el archivo entero consumiendo mucha data y tiempo. | - |
| **Requerimientos Físicos** | Mínimos. Motor alojado en placas con ínfima memoria RAM (<8MB). | PC Windows moderna, licencias. | Smartphone moderno y plan de datos. |

---

##  4. Requerimientos del Sistema (Ingeniería de Software)

### Requerimientos Funcionales (RF)
*   **RF01 (Registro Paralelo):** El sistema debe permitir el acceso y manipulación concurrente de registros médicos por múltiples usuarios simultáneamente sin bloqueos de aplicación.
*   **RF02 (Gestión Offline Ininterrumpida):** Todo el módulo de Historia Clínica (Lectura/Escritura/Edición) debe dar un tiempo de respuesta inmediato en condiciones 100% desconectadas del internet global.
*   **RF03 (Bitácora de Inmortalidad - WAL):** El motor local deberá implementar una bitácora "Append-Only" en la ROM (Disco) persistiendo cada mandato antes de insertarlo en RAM, para salvaguardar todos los registros ante cortes de energía directos.
*   **RF04 (Sincronización Eficiente Edge-To-Core):** Al detectar conectividad al servidor central del MINSA (Ministerio de Salud), el sistema ejecutará un "Delta Sync", resolviendo conflictos y empujando únicamente las variaciones locales hacia la Nube.
*   **RF05 (Radiodifusión de Alertas):** Funcionalidad `Pub/Sub` interna para que una emergencia reportada localice de inmediato a todo el personal conectado a la Intranet.

### Requerimientos No Funcionales (RNF)
*   **RNF01 (Eficiencia de Memoria):** El motor nativo (RuralKV) está estrictamente restringido operativamente y usará un *Arena Allocator* para asegurar que no se produzcan fugas ni colapsos al manejar alto volumen histórico.
*   **RNF02 (Confiabilidad Paralela):** Manejo seguro de memoria protegida por bloqueos mutuos (*Mutexes / Pthreads*) eliminando Condiciones de Carrera (Race conditions).
*   **RNF03 (Escalabilidad de Hardware Pobre):** El núcleo debe ser independiente de S.O. robustos, completamente operable mediante llamadas del estándar POSIX, garantizando su fluidez desde un microcontrolador antiguo hasta un microprocesador moderno.

---

##  5. Anatomía de la Infraestructura Oculta (El Motor RuralKV)

Para que *ImmortalData* triunfe, tuvimos que bajar al metal y programar el corazón lógico en C. Esta es la arquitectura ingenieril técnica que sostiene a la plataforma de salud:

**Tecnología Base:**
*   Lenguaje Primitivo: **C estándar (C11)**
*   Multihilo: **POSIX Threads (Pthreads)**
*   Enrutamiento: **POSIX Sockets**

**Estructuras Centrales de RuralKV:**
1.  **Arena Allocator:** Patrón de diseño de gestión en RAM para evitar fragmentación y ser rápido.
2.  **Tabla Hash Encadenada:** Nuestro indexador O(1) de claves y valores (ej. "Paciente1" -> Exp. Médico).
3.  **Sistema Write-Ahead Log (WAL):** Registra linealmente cada modificación en binario dentro del disco crudo antes de ejecutar el cambio en memoria, logrando tolerancia a caídas absolutas.
4.  **Pub/Sub Event Bus:** Matriz de distribución de notificaciones a los sockets suscritos.

---

##  6. Arquitectura Frontend (Interfaz Ligera)

Dado que el hardware que corre el nodo no tiene los recursos para sostener intérpretes pesados (como Node.js o Python) ni navegadores consumiendo Gigabytes de RAM, la capa de presentación ha sido elegida estratégicamente bajo el principio de "Cero Grasa":

**Tecnología Elegida: HTMX + Alpine.js con Servidor C Nativo**

*   **HTML Mágico:** HTMX permite que el HTML puro envíe peticiones HTTP directamente al motor C sin escribir lógica pesada en JavaScript.
*   **Consumo Casi Nulo:** Toda la interfaz final distribuida al usuario pesa menos de **15 KB**, permitiendo integrarse dentro de un archivo base y cargar instantáneamente.
*   **Conexión Inyectada:** En lugar de crear un backend web intermedio, nuestro motor RuralKV servirá los puertos HTTP directamente, garantizando latencia cercana a los 0ms entre el Clic y el Guardado.
*   **Vida para hardware viejo:** Al no usar Virtual DOM (como React), la carga de la memoria de video y CPU en las tablets y celulares rurales es asombrosamente baja, previniendo sobrecalentamientos y protegiendo las baterías.

---

##  Proyecto de Feria
Desarrollado para la **Feria de Ingeniería de Software - UNI**. 
(Motor creado en C, caso de uso conceptualizado para el Perú profundo).
