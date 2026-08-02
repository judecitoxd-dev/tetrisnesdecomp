# Tetris NES — ports nativos para PC y Android

Versión **0.14** de una reimplementación portable de **Tetris (USA) para NES**,
escrita en C99. El repositorio no distribuye la ROM, gráficos, música ni datos
extraídos; cada usuario proporciona su copia legal.

## Novedades de v0.14

- Máquina incremental de la catedral B-Type basada en los contadores del 6502.
- Movimiento, animación y activación en cascada desde las tablas originales.
- Prueba de 43,200 estados: diez niveles, seis alturas y 720 fotogramas.
- Nueva herramienta `tetris_apu_scenario` para ocho efectos originales.
- Nueva herramienta `tools/apu_matrix.py` para capturar y comparar:
  - diez pistas musicales;
  - movimiento, rotación, bloqueo, línea, Tetris y subida de nivel;
  - derrota y final completado.
- Manifiesto JSON con hashes, ciclos, fotogramas y actividad APU.
- Detección del primer caso, fotograma y registro divergente frente a Mesen.
- Herramientas y paquetes actualizados para Windows, Linux y Android.

La entrada móvil de la catedral ya no figura como pendiente. El APU dispone de
una matriz completa de validación, aunque las capturas de referencia deben ser
generadas localmente por el usuario.

## Estado actual

- Modos A y B jugables a 60.0988 actualizaciones por segundo.
- Demo original desde comandos y piezas de la ROM.
- Menús, récords y finales reconstruidos desde PPU, CHR y OAM.
- Catedral B-Type animada mediante su máquina de estados original.
- Música y efectos originales interactivos desde la ROM legal.
- CPU y APU sincronizados por ciclos NTSC.
- Repeticiones deterministas y comparación de trazas.
- Teclado, controles táctiles y gamepad.
- Windows, Linux, ARM64 y ARMv7 sobre el mismo núcleo.

Todavía no es una decompilación bit a bit. Quedan la microtemporización de bus,
la entrega asíncrona de IRQ, diferencias de RAM/PPU de la demo y una
construcción 6502 enlazable.

## Ejecutar el port de PC

```powershell
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes"
```

## Generar una pista

```bash
tetris_apu_render "Tetris (USA).nes" 1 60 music-1.wav track-01.csv
```

## Generar un efecto

```bash
tetris_apu_scenario "Tetris (USA).nes" rotate 180 effect-rotate.csv
```

Escenarios disponibles:

```text
move rotate lock line tetris level game-over complete
```

## Generar la matriz completa

```bash
python tools/apu_matrix.py capture \
  --apu-render ./tetris_apu_render \
  --apu-scenario ./tetris_apu_scenario \
  --rom "Tetris (USA).nes" \
  --output port-matrix
```

Compararla con una matriz de referencia:

```bash
python tools/apu_matrix.py compare mesen-matrix port-matrix \
  --json apu-matrix-report.json
```

La herramienta espera 18 casos y compara por defecto ciclos, stalls, IRQ y
escrituras `$4000-$4017`.

## Captura de referencia con Mesen

1. Abre la ROM legal en Mesen 2 o Mesen CE.
2. Permite acceso Lua a I/O y funciones del sistema.
3. Ejecuta `tools/mesen_trace.lua`.
4. Captura cada pista o efecto que vas a incorporar a la matriz de referencia.

Las capturas, WAV y ROM permanecen fuera del repositorio.

## Compilar y probar PC

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

## Android

```bash
cd android
gradle --no-daemon :app:assembleDebug
```

El APK usa el mismo núcleo C99 y no incluye ROM, audio renderizado ni trazas.

## Documentación

- [`docs/PORT_STATUS.md`](docs/PORT_STATUS.md)
- [`docs/APU_RENDERING.md`](docs/APU_RENDERING.md)
- [`docs/TRACE_COMPARISON.md`](docs/TRACE_COMPARISON.md)
- [`docs/ROM_DEMO_OAM.md`](docs/ROM_DEMO_OAM.md)
- [`docs/ROM_TYPE_A_ENDING.md`](docs/ROM_TYPE_A_ENDING.md)
- [`docs/ROM_SCREENS.md`](docs/ROM_SCREENS.md)
- [`docs/ROM_MAP.md`](docs/ROM_MAP.md)
- [`docs/ANDROID_PORT.md`](docs/ANDROID_PORT.md)
- [`docs/REPLAY_FORMAT.md`](docs/REPLAY_FORMAT.md)
- [`docs/AUDIO_PACK.md`](docs/AUDIO_PACK.md)
- [`docs/DECOMP_TOOLS.md`](docs/DECOMP_TOOLS.md)

## Legalidad

El proyecto contiene código original, herramientas y documentación. No contiene
ROM, PRG/CHR extraído, música de Nintendo, capturas de RAM, WAV generados ni
tablas de audio extraídas.
