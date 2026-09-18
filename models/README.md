# Propuesta de deteccion facial en SD

## Objetivo

Leer las imagenes JPG almacenadas por la camara en `/DCIM`, detectar caras en el ESP32-S3 y generar una miniatura anotada que se pueda consultar desde la galeria web.

La prioridad es la precision. El tiempo de procesamiento por imagen puede ser alto porque el ESP32-S3 dispone de PSRAM.

## Modelo seleccionado

Archivo local:

`yolov5face_converted.tflite`

Fuente de evaluacion:

`DaoDacDat/YOLOv5Face_Tflite`

Caracteristicas verificadas:

- Tamano: 2.56 MiB.
- Formato: TFLite FlatBuffer (`TFL3`).
- Entrada: `[1, 3, 640, 640]` en `float32`.
- Salida: `[1, 25200, 16]` en `float32`.
- Detector especifico de rostros, no el modelo COCO de personas.
- SHA-256: `80FE8E6A99D9FF253A4E732EFD831CDCC9DA53AB5DA5221AF940BF403CC5AD60`.

El modelo necesita PSRAM para el tensor de entrada, la salida y el tensor arena. El repositorio de origen no declara una licencia claramente; debe revisarse antes de distribuir el producto.

## Flujo propuesto

```text
SD compartida con la camara
	-> la camara libera el bus
	-> montaje de SD_MMC
	-> busqueda de una JPG nueva en /DCIM
	-> decodificacion de la imagen original
	-> letterbox a 640x640
	-> conversion RGB CHW float32
	-> inferencia YOLO
	-> confianza + NMS
	-> coordenadas en la imagen original
	-> salida Serial
	-> dibujo de cajas en la miniatura
	-> guardado en /THUMBS
	-> galeria web
```

La inferencia debe ejecutarse sobre la JPG original. Las miniaturas de 420 px son para navegacion web y pueden perder caras pequenas.

## Salida Serial

El firmware debe emitir una linea por imagen y una por cara:

```text
[FACE] /DCIM/100MEDIA/IMG0001.JPG
[FACE] count=2
[FACE] #0 confidence=0.91 x=842 y=214 w=176 h=190
[FACE] #1 confidence=0.78 x=1320 y=260 w=151 h=164
[FACE] thumbnail=/THUMBS/3A91F210.jpg
```

Cuando no haya detecciones:

```text
[FACE] /DCIM/100MEDIA/IMG0002.JPG
[FACE] count=0
```

## Anotacion de miniaturas

Las coordenadas devueltas por el modelo deben deshacer primero el `letterbox` y quedar expresadas en pixeles de la JPG original. Luego se escalan a las dimensiones reales de la miniatura y se dibuja una caja por cara antes de codificar el JPEG final.

La galeria web seguira sirviendo la miniatura desde `/THUMBS`; por tanto, los rectangulos quedan visibles sin cambiar el formato de la galeria.

## Integracion Arduino

La implementacion se divide en estas piezas:

- `sd_scanner`: recorre `/DCIM` y evita reprocesar archivos.
- `image_processor`: decodifica JPEG, hace letterbox y crea miniaturas.
- `edge_inference`: carga el modelo, prepara tensores, ejecuta YOLO y aplica NMS.
- `SDTRAP_EdgeSD.ino`: coordina el ciclo, Serial y el estado de la SD.
- `sd_bus`: garantiza que la camara haya liberado el bus antes de leer.

El modelo puede empaquetarse en el firmware como array C o cargarse desde la SD. Para el primer prototipo conviene cargarlo desde un archivo controlado y medir memoria antes de fijar la estrategia de despliegue.

## Memoria y rendimiento

La entrada `3 x 640 x 640 x 4` requiere aproximadamente 4.7 MiB. La salida `25200 x 16 x 4` requiere aproximadamente 1.6 MiB, sin contar el tensor arena ni los buffers JPEG. Estos datos justifican el uso de PSRAM y la liberacion estricta de buffers temporales.

Primera fase: ejecutar el modelo `float32` para validar el postprocesado y las cajas. Segunda fase: generar una variante INT8 con un conjunto representativo de imagenes de camara, comparar precision y reducir el consumo de memoria.

## Criterios de aceptacion

- La SD se monta sin interferir con la camara.
- Se detectan JPG en subdirectorios de `/DCIM`.
- Una imagen con una cara produce al menos una linea `[FACE]` con confianza y coordenadas.
- La miniatura contiene una caja visible sobre la cara detectada.
- Una imagen sin caras produce `count=0`.
- La misma JPG no se procesa repetidamente.
- El sistema informa por Serial si falta PSRAM, memoria o el modelo.
