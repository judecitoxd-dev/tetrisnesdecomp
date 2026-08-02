# Renderizado automático e interactivo del APU original

La v0.12 usa la ROM legal del usuario para ejecutar el controlador de sonido
6502 de Tetris tanto en la herramienta WAV como dentro del juego de PC y
Android. El repositorio no contiene música, muestras, tablas extraídas ni audio
pregrabado.

## Arquitectura

1. `cpu6502.c` ejecuta las instrucciones oficiales NMOS 6502 usadas por el
   controlador.
2. `rom_audio.c` mapea los 32 KiB de PRG en `$8000-$FFFF`, 2 KiB de RAM y los
   registros APU `$4000-$4017`.
3. La inicialización llama `$E006` y el callback de audio ejecuta `$E000` una vez
   por fotograma NTSC.
4. `nes_apu.c` produce dos pulsos, triángulo, ruido y DMC a 48 kHz.
5. `audio.c` entrega esas muestras directamente a SDL en Windows, Linux y
   Android.
6. `tetris_apu_render` conserva la ruta offline para WAV y comparación de
   trazas.

Los offsets solo se habilitan para la ROM comprobada CRC32 `D16EA396`. Una ROM
compatible no verificada sigue usando el sintetizador alternativo.

## Audio durante el juego

Cuando se carga la ROM exacta, el backend visible es `ROM APU`:

- MUSIC 1 selecciona el comando original 3;
- MUSIC 2 selecciona el comando original 4;
- MUSIC 3 selecciona el comando original 5;
- al alcanzar la zona alta del tablero se cambia a 6, 7 u 8;
- MUSIC OFF escribe `$FF` en `musicTrack` (`$06F5`).

Los eventos del núcleo C se conectan a las órdenes originales:

| Evento del port | Slot | Comando |
|---|---:|---:|
| Movimiento | `$06F1` | 3 |
| Tetris | `$06F1` | 4 |
| Rotación | `$06F1` | 5 |
| Subida de nivel | `$06F1` | 6 |
| Bloqueo | `$06F1` | 7 |
| Línea | `$06F1` | 10 |
| Derrota | `$06F0` | 2 |
| Final completado | `$06F5` | 2 |

Partidas normales, demo y replays usan el mismo backend. Al cambiar la ROM, el
driver anterior se desconecta antes de liberar su PRG para evitar accesos a
memoria inválida.

## Prioridad de backends

1. Paquete OGG del usuario, cuando está configurado.
2. `ROM APU`, cuando el CRC y el driver son válidos.
3. Sintetizador alternativo incorporado.

Un fallo del intérprete durante el callback desactiva `ROM APU` y permite que el
sintetizador continúe sin cerrar la aplicación.

## Generar un WAV

```bash
tetris_apu_render "Tetris (USA).nes" 1 60 music-1.wav
```

Parámetros:

- pista: `1` a `10`, tal como espera `musicTrack`;
- duración: segundos, hasta una hora;
- salida: WAV mono, 48 kHz y 16 bits.

## Generar y comparar una traza APU

```bash
tetris_apu_render "Tetris (USA).nes" 1 60 music-1.wav port-apu.csv
python tools/trace_compare.py mesen-apu.csv port-apu.csv --columns apu_writes
```

La columna `apu_writes` usa secuencias como
`4000=9F|4002=FD|4003=08`. La primera diferencia muestra el fotograma exacto en
el que el driver embebido deja de coincidir con el emulador.

## Captura automática con Mesen

1. Abre la ROM legal en Mesen 2 o Mesen CE.
2. Abre la ventana Lua y permite acceso a I/O y funciones del sistema.
3. Ejecuta `tools/mesen_trace.lua`.
4. Inicia la demo o la escena que deseas comparar.

El script crea `tetris-reference.csv` y `tetris-apu-writes.csv` en la carpeta de
datos Lua.

## Autopruebas sin ROM

```bash
tetris_apu_render --self-test
ctest --test-dir build -C Release --output-on-failure
```

Las pruebas usan un PRG artificial. Comprueban muestras no silenciosas, órdenes
de pista, apagado y los efectos conectados; no contienen bytes del juego.

## Límites actuales

- El intérprete todavía no devuelve el número real de ciclos consumidos por
  cada instrucción.
- El APU avanza a nivel de muestra y no con cada ciclo 2A03.
- Frame IRQ y DMC IRQ no llegan todavía a la CPU.
- DMC no modela sus robos de ciclos.
- Falta comparar automáticamente todas las pistas, variantes allegro y efectos
  contra una captura Mesen completa.

Estos límites impiden denominarlo exacto por ciclo, aunque la ruta interactiva
y la generación automática desde la ROM ya están implementadas.
