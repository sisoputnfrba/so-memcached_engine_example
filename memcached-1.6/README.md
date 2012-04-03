
# Intro: #

Coloque esta carpeta dentro de su workspace. Importar el proyecto con el eclipse usando la opción: Import -> Existing Project in Workspace y seleccionar esta carpeta.

Para poder ejecutar memcached con eclipse, debe crear un nuevo Run Configuration. Dentro de la configuración debe colocar:

En la sopala main:

* Project: memcached-1.6
* C/C++ Application: .libs/memcached

En la solapa de Arguments:

* -vv -p 11212 -E .libs/default_engine.so

En la solapa de Enviroment:

* Agregar una nueva variable de entorno "LD_LIBRARY_PATH" con el valor .libs/

# Compilación: #

Ejecute el comando 'make' en la carpeta. Este generara una carpeta llamada .libs donde se encontraran todos los binarios y bibliotecas compartidas.

# Ejecución: #

Si bien los binarios y bibliotecas compartidas se encuentran en .libs, para correro hay que correr el script llamado memcached que se encuentra dentro de esta carpeta. Este se encarga de configurar los path para levantar las bibliotecas compartidas que utiliza memached:

./memcached -p 11212 -E .libs/default_engine.so


# Memcached Wiki: #

   http://www.memcached.org/wiki

El protocolo ASCII esta documentado en doc/protocol.txt

# Dependencias: #

   -- libevent, http://www.monkey.org/~provos/libevent/ (libevent-dev)

