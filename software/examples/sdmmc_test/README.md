# Prueba SD independiente

Sketch para ESP32-S3, Arduino IDE y la biblioteca SD_MMC incluida en Arduino-ESP32.
Primera etapa del proyecto, antes del hardware de SD compartida con la camara.

## Preparacion

1. Abrir `sdmmc_test.ino` y seleccionar la placa ESP32-S3 correspondiente.
2. Asignar `SD_CLK`, `SD_CMD` y `SD_D0` a GPIO libres de esa placa. Los valores
   iniciales `-1` impiden iniciar el bus hasta definir el cableado real.
3. Conectar una SD directamente al ESP32-S3, con alimentacion de 3,3 V, masa comun
   y las senales CLK, CMD y DAT0. Usar un zocalo/adaptador que exponga SD_MMC;
   un modulo solo SPI no es equivalente. Definir los pull-ups de CMD y DAT0-DAT3
   segun la referencia de Espressif, incluso para las lineas de datos no utilizadas.
   La seleccion definitiva de GPIO y circuito queda pendiente del hardware real.
4. Usar una tarjeta con un sistema de archivos FAT compatible; para la prueba
   inicial, una tarjeta ya preparada en FAT32. El sketch nunca solicita formatearla.
5. Cargar el sketch y abrir Serial a 115200 baudios. Si se utiliza USB nativo,
   configurar USB CDC segun la placa seleccionada. Reiniciar para repetir.

## Que comprueba

- Montaje SD_MMC en modo 1 bit a 10 MHz e informacion de capacidad.
- Listado de la raiz y hasta tres niveles de subdirectorios, limitado a 200 entradas.
- Lectura completa del primer archivo `.jpg` o `.jpeg` encontrado en ese listado,
  mediante un buffer de 1 KiB. Comprueba los bytes leidos, sin decodificar ni validar
  la integridad del JPEG. Si no encuentra una imagen, informa `[SKIP]`.
- Desmontaje al terminar. No hay Wi-Fi ni inferencia en este sketch.

## Escritura opcional

Con `ENABLE_WRITE_TEST = true`, crea `/SDTRAP_TEST.bin`, escribe 4096 bytes de
patron conocido, cierra, reabre, verifica cada byte y elimina el archivo creado.
Si esa ruta ya existe, informa un error y no la modifica. Un corte de energia puede
dejar ese archivo: revisarlo y retirarlo manualmente antes de repetir la prueba.
Por defecto, la prueba solo realiza operaciones de lectura sobre los archivos.

La prueba requiere acceso exclusivo a la SD; no implementa arbitraje con la camara.
El hardware compartido se desarrollara despues de validar esta conexion directa.

## Resultado esperado

Mensajes `[OK]` para montaje, lectura (si hay JPG) y escritura (si esta habilitada),
seguidos de `SD desmontada`. Los errores se identifican con `[ERROR]` y las pruebas
omitidas con `[SKIP]`. Una prueba omitida no valida esa funcionalidad.

## Referencias

- [Ejemplo oficial SD_MMC](https://github.com/espressif/arduino-esp32/blob/master/libraries/SD_MMC/examples/SDMMC_Test/SDMMC_Test.ino)
- [API SD_MMC](https://github.com/espressif/arduino-esp32/blob/master/libraries/SD_MMC/src/SD_MMC.h)
