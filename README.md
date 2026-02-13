# Sistema SITREP P2P con Protocolo DDS y SQLite (C++)

  Este proyecto implementa un sistema de comunicación de Reportes Tácticos (SITREP) en tiempo real entre nodos distribuidos utilizando el middleware **OpenDDS**. 

  La arquitectura es **Peer-to-Peer (P2P)** utilizando el protocolo de interoperabilidad **RTPS** (Real-Time Publish-Subscribe). El sistema integra una base de datos local (**SQLite3**) para registrar de manera persistente la información táctica, y maneja el ciclo de vida completo de los datos (publicación y baja distribuida).

### Requisitos Previos

  Para compilar y ejecutar este proyecto, necesitas tener instalado y configurado **OpenDDS** en un entorno Linux (probado en Ubuntu/WSL). 

  Además, este proyecto requiere las librerías de desarrollo de SQLite para gestionar la base de datos. Asegúrate de instalarlas ejecutando:
```bash
sudo apt update
sudo apt install sqlite3 libsqlite3-dev
```
Asegúrate de tener cargadas las variables de entorno de DDS antes de empezar:

```Bash
source ~/OpenDDS/setenv.sh
```
## Compilación
Sigue estos pasos para generar los binarios y archivos de soporte:

### Clonar el repositorio:

```Bash
git clone https://github.com/GalliMart1/base_de_datos.git
cd base_de_datos
```
### Generar el Makefile:
Utilizamos MPC (Make Project Creator) para generar los archivos de compilación basados en el workspace y el tipo de sistema (GNU Make).

```Bash
$ACE_ROOT/bin/mwc.pl -type gnuace
```
### Compilar:

```Bash
make
```
Esto generará el código base del IDL y el ejecutable principal del nodo táctico.

## Ejecución
Para probar la comunicación, simula al menos dos nodos. El proyecto utiliza el archivo rtps.ini para la configuración de descubrimiento e interoperabilidad sobre la red.

**Terminal 1 (Nodo Alfa):** 

```Bash
./sitrep_node -DCPSConfigFile rtps.ini
```
**Terminal 2 (Nodo Bravo):** 

```Bash
./sitrep_node -DCPSConfigFile rtps.ini
```
  *Nota: Al iniciar, el sistema te pedirá que ingreses un nombre para identificar la terminal actual en la red.*

## Uso del Sistema
El programa funciona de manera concurrente como Publicador (DataWriter) y Suscriptor (DataReader). Cuenta con un menú interactivo protegido contra errores de tipeo.

**Para Publicar un SITREP:**

- Escribe **p** en la consola y presiona **Enter**.

**El sistema te solicitará los siguientes datos tácticos:**

- Track ID: (Número identificador único).
- Identidad: (Ej: HOSTIL / AMIGO).
- Latitud y Longitud: (Coordenadas numéricas).
- Info Extra: (Cadena de texto con detalles adicionales).

*Verás el mensaje >>> SITREP ENVIADO <<<.*

**Para Dar de Baja / Eliminar un SITREP:**

- Escribe la tecla correspondiente en tu menú (ej: b para borrar) y presiona **Enter**.
- Ingresa el Track ID del reporte que deseas eliminar.

*El sistema registrará la instancia y enviará una señal de dispose a través de la red DDS. Esto notifica a todos los demás nodos que el reporte ya no está activo, y simultáneamente lo elimina de tu base de datos local SQLite (>>> ORDEN DE BAJA ENVIADA A LA RED <<<).*

**Para Salir:**

- Escribe **q** y presiona Enter para cerrar el participante DDS de forma segura.

**Consulta de la Base de Datos:**
- Cada vez que un nodo recibe un SITREP, este se almacena en la base de datos local tactical_data.db. Cuando se recibe una orden de baja, el registro se elimina.

**Para visualizar el estado actual de los reportes tácticos almacenados desde tu terminal, utiliza:**

```Bash
sqlite3 tactical_data.db "SELECT * FROM sitreps;"
```
## Estructura del Proyecto
- *Sitrep.idl:* Definición de la estructura de datos SitrepMessage. Contiene los campos del reporte, definiendo el trackId como clave principal (key) para el manejo de instancias.
- *main.cpp:* Código fuente principal. Maneja el bucle de interacción, la publicación de mensajes y la lógica de ciclo de vida de DDS (register_instance y dispose) para dar de baja reportes.
- *DatabaseManager.cpp / .h:* Encargado de la conexión con la base de datos SQLite y de la ejecución de consultas (inserción y borrado de trackId).
- *DataReaderListenerImpl.cpp / .h:* Listener del suscriptor. Captura los datos entrantes de la red y las señales de estado para mantener sincronizada la base de datos.
- *rtps.ini:* Archivo de configuración para OpenDDS (transporte y descubrimiento P2P).
