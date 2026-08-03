# v0.22 — audio continuo por ciclos

La reserva `TETRIS_AUDIO_RING_TARGET` solo controla cuantas muestras se mantienen preparadas. No cambia tono, tempo ni la forma de onda.

El renderer anterior avanzaba el APU durante la rutina 6502 sin capturar muestras y luego repartia todas las muestras del fotograma sobre los ciclos idle. v0.22 sustituye ese modelo por una linea temporal continua: cada ciclo de CPU, incluidos driver y stalls DMC, participa en el reloj de muestreo de 48 kHz.

Objetivos verificables:

- producir muestras durante todos los ciclos del fotograma;
- conservar exactamente el promedio NTSC de 1789773 Hz y 60.0988 fps;
- no alterar el tono al cambiar el tamano del ring buffer;
- mantener el callback libre de emulacion;
- comparar duracion, frecuencia y trazas contra una captura Mesen local.
