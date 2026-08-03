# Caché OGG generado desde la ROM legal

El caché OGG evita que el intérprete 6502/APU tenga que generar música en
tiempo real. Las pistas y efectos se renderizan una sola vez desde la ROM legal
del usuario, se convierten localmente a Ogg Vorbis y el port los reproduce
mediante SDL2_mixer.

El repositorio no contiene ROM, WAV, OGG, tablas musicales ni audio extraído.
La carpeta generada es privada del usuario y no debe subirse al repositorio.

## Bucles exactos desde el driver de la ROM

La versión 0.28 del generador ya no crea pistas de duración fija que regresan al
principio completo. Cada pista se renderiza durante al menos 120 segundos y se
guarda una traza por fotograma de las escrituras APU producidas por el bytecode
musical original.

El analizador:

1. elimina únicamente escrituras de mantenimiento de envolvente y contador de
   frame que no identifican el avance de la partitura;
2. conserva cambios de tono, instrumento, sweep, ruido y DMC;
3. busca dos periodos completos e idénticos del flujo de eventos;
4. convierte los límites de fotograma a muestras PCM usando el reloj NTSC de
   1,789,773 Hz y la salida de 48,000 Hz;
5. recorta el OGG exactamente al final del primer ciclo completo;
6. escribe comentarios Vorbis `LOOPSTART` y `LOOPEND` en posiciones de muestra.

SDL_mixer 2.8.1, la versión usada por el proyecto, reconoce esas etiquetas y
salta automáticamente desde `LOOPEND` hasta `LOOPSTART`. Por eso la introducción
se reproduce una sola vez y la sección repetitiva continúa sin volver a iniciar
la canción completa.

Las pistas 1 y 2 son secuencias de una sola ejecución. Se recortan al primer
fotograma estable posterior al final de su script y no reciben etiquetas de
bucle. Las pistas 3 a 10 sí reciben puntos de bucle.

## Resultados verificados para Tetris (USA), CRC32 D16EA396

| Pista | Tipo | Inicio | Final | Duración del ciclo |
|---:|---|---:|---:|---:|
| 1 | una ejecución | 0 | muestra 1,227,578 | 25.574542 s |
| 2 | una ejecución | 0 | muestra 111,017 | 2.312854 s |
| 3 | bucle | muestra 798 | muestra 2,626,874 | 54.709917 s |
| 4 | bucle | muestra 77,472 | muestra 1,764,294 | 35.142125 s |
| 5 | bucle | muestra 13,577 | muestra 1,163,683 | 23.960542 s |
| 6 | bucle allegro | muestra 798 | muestra 1,313,836 | 27.354958 s |
| 7 | bucle allegro | muestra 51,914 | muestra 1,176,462 | 23.428083 s |
| 8 | bucle allegro | muestra 8,785 | muestra 775,522 | 15.973688 s |
| 9 | bucle | muestra 32,746 | muestra 1,566,220 | 31.947375 s |
| 10 | bucle | muestra 798 | muestra 614,188 | 12.779000 s |

Estos valores no están codificados como audio ni como una tabla fija del juego.
El generador vuelve a derivarlos de las trazas producidas por la ROM entregada
por el usuario y los registra en `audio-cache.json`.

## Contenido generado

`tools/build_audio_cache.py` crea:

- `track_01.ogg` a `track_10.ogg`: las diez selecciones del driver original;
- `music_1.ogg`, `music_2.ogg`, `music_3.ogg`: alias de las tres canciones del
  selector normal, conservando sus etiquetas de bucle;
- `move.ogg`, `rotate.ogg`, `lock.ogg`, `line.ogg`, `tetris.ogg`;
- `level_up.ogg`, `game_over.ogg`, `complete.ogg`;
- una traza CSV para cada pista y efecto;
- `audio-cache.json` con SHA-256, CRC, hashes APU, muestras codificadas y puntos
  exactos de introducción/bucle.

Los efectos se renderizan con `--isolated`: no incluyen música de fondo.

## Requisitos

1. Compilar `tetris_apu_render` y `tetris_apu_scenario`.
2. Tener Python 3.
3. Tener `ffmpeg` con el encoder `libvorbis`.
4. Proporcionar una copia legal compatible de `Tetris (USA).nes`.

## Windows

Desde la carpeta del proyecto compilado:

```powershell
python tools\build_audio_cache.py `
  --rom "C:\ROMs\Tetris (USA).nes" `
  --bin-dir build\Release `
  --overwrite
```

La salida predeterminada coincide con la carpeta `audio` de preferencias que el
port busca automáticamente. También puede elegirse una carpeta al lado del
ejecutable:

```powershell
python tools\build_audio_cache.py `
  --rom "C:\ROMs\Tetris (USA).nes" `
  --bin-dir build\Release `
  --output ".\audio" `
  --overwrite

.\tetris_pc.exe --rom "C:\ROMs\Tetris (USA).nes" --audio-pack ".\audio"
```

## Linux

```bash
python3 tools/build_audio_cache.py \
  --rom "$HOME/ROMs/Tetris (USA).nes" \
  --bin-dir build \
  --output ./audio \
  --overwrite
```

## Parámetros

- `--music-seconds 180`: material usado para demostrar al menos dos ciclos. Se
  acepta de 120 a 3600 segundos.
- `--effect-frames 240`: duración capturada para cada efecto.
- `--quality 5`: calidad Vorbis entre 0 y 10.
- `--keep-wav`: conserva los WAV completos usados durante el análisis.
- `--output`: carpeta de salida.
- `--ffmpeg`: ruta explícita al ejecutable.
- `--overwrite`: permite regenerar un paquete existente.

## Verificación

La prueba `tools/build_audio_cache.py --self-test` comprueba:

- normalización de escrituras APU;
- detección de una introducción seguida por dos ciclos idénticos;
- detección y recorte de una pista de una sola ejecución;
- conversión determinista de fotogramas a muestras;
- contenido esperado del paquete.

Además, `ffprobe` puede mostrar las etiquetas incluidas:

```bash
ffprobe -v error -show_entries stream_tags=TITLE,LOOPSTART,LOOPEND \
  audio/track_03.ogg
```

## Diferencias que todavía quedan

Los bucles de archivo ya no son una aproximación de duración fija. Aun así, la
identidad total de audio requiere:

- comparar las escrituras y el PCM contra capturas de referencia de Mesen;
- aplicar cada escritura APU en el ciclo exacto de bus, no solo dentro de la
  temporización física de la instrucción;
- comprobar en equipos reales que el contador de underruns permanece en cero;
- completar el mismo camino OGG en todas las variantes Android.

El caché mejora la continuidad y conserva la introducción real, pero no se
presenta todavía como identidad binaria o eléctrica completa con una NES.
