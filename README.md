# Tetris NES — ports nativos para PC y Android

Versión **0.13** de una reimplementación portable de **Tetris (USA) para NES**,
escrita en C99. El proyecto no distribuye la ROM, gráficos, música ni datos
extraídos: cada usuario proporciona su copia legal.

## Novedades de v0.13

- El intérprete 6502 cuenta los ciclos de las instrucciones oficiales usadas.
- Ramas tomadas y lecturas indexadas aplican penalizaciones de ciclo y página.
- El APU avanza con el reloj NTSC de CPU, no solamente con el número de muestras.
- Pulso, triángulo, ruido y DMC usan temporizadores por ciclos.
- El secuenciador de cuatro/cinco pasos activa envolventes, longitudes y barridos.
- Frame IRQ y DMC IRQ aparecen en `$4015`.
- Las lecturas DMC contabilizan robos de cuatro ciclos.
- Cada fotograma recibe aproximadamente 29,780.5 ciclos y 798.7 muestras.
- Las trazas añaden ciclos del fotograma, ciclos del driver, stalls e IRQ.
- Windows, Linux y Android usan la misma implementación portable.

El audio original ya es interactivo y guiado por ciclos. Todavía falta ubicar
cada escritura en su ciclo exacto de bus y demostrar igualdad completa contra
Mesen para todas las pistas y efectos.

## Estado actual

- Modos A y B jugables a 60.0988 actualizaciones por segundo.
- Demo original desde comandos y piezas de la ROM.
- Menús, récords y finales reconstruidos desde PPU, CHR y OAM.
- Teclado, controles táctiles y gamepad.
- Repeticiones deterministas y comparación de trazas por fotograma.
- Música y efectos originales interactivos desde la ROM legal.
- CPU y APU sincronizados por ciclos NTSC.
- Renderer WAV y traza APU automática para análisis offline.
- Paquetes OGG opcionales y sintetizador de respaldo.

Todavía no es una decompilación bit a bit. Quedan la microtemporización del bus,
la entrega asíncrona de IRQ, diferencias de RAM/PPU, parte del movimiento de la
catedral B-Type y una construcción 6502 enlazable.

## Ejecutar el port de PC

```powershell
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes"
```

Cuando la ROM coincide con CRC32 `D16EA396`, la consola informa:

```text
Audio backend: ROM APU
```

## Generar audio y traza desde la ROM

```bash
tetris_apu_render "Tetris (USA).nes" 1 60 music-1.wav port-apu.csv
```

La traza incluye:

```text
frame,cpu_cycles,driver_cycles,dmc_stall_cycles,irq,apu_writes
```

Comparación con una captura local de Mesen:

```bash
python tools/trace_compare.py mesen-apu.csv port-apu.csv --columns apu_writes
```

Las pistas aceptadas son `1` a `10`.

## Captura automática con Mesen

1. Abre tu ROM legal en Mesen 2 o Mesen CE.
2. Abre la ventana Lua y permite acceso a I/O y funciones del sistema.
3. Ejecuta `tools/mesen_trace.lua`.
4. Inicia la demo o escena que deseas comparar.

El script crea `tetris-reference.csv` y `tetris-apu-writes.csv` dentro de la
carpeta de datos del script.

## Compilar y probar PC

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Las pruebas comprueban ciclos CPU, secuenciador, IRQ, stalls DMC y audio no
silencioso usando exclusivamente un PRG artificial.

## Android

```bash
cd android
gradle --no-daemon :app:assembleDebug
```

El APK compila el intérprete 6502, el APU por ciclos y el driver original para
`arm64-v8a` y `armeabi-v7a`. La ROM se selecciona mediante el selector del
sistema y permanece en el almacenamiento privado de la aplicación.

## Documentación

- [`docs/PORT_STATUS.md`](docs/PORT_STATUS.md)
- [`docs/APU_RENDERING.md`](docs/APU_RENDERING.md)
- [`docs/TRACE_COMPARISON.md`](docs/TRACE_COMPARISON.md)
- [`docs/ROM_MAP.md`](docs/ROM_MAP.md)
- [`docs/ROM_SCREENS.md`](docs/ROM_SCREENS.md)
- [`docs/ROM_DEMO_OAM.md`](docs/ROM_DEMO_OAM.md)
- [`docs/ROM_TYPE_A_ENDING.md`](docs/ROM_TYPE_A_ENDING.md)
- [`docs/ANDROID_PORT.md`](docs/ANDROID_PORT.md)
- [`docs/REPLAY_FORMAT.md`](docs/REPLAY_FORMAT.md)
- [`docs/AUDIO_PACK.md`](docs/AUDIO_PACK.md)
- [`docs/DECOMP_TOOLS.md`](docs/DECOMP_TOOLS.md)

## Legalidad

El repositorio contiene código original, herramientas y documentación. No
contiene ROM, PRG/CHR extraído, imágenes, música de Nintendo, capturas de RAM,
WAV generados ni tablas de audio del cartucho.
