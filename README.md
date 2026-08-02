# Tetris NES — ports nativos para PC y Android

Versión **0.20** de una reimplementación portable de **Tetris (USA) para NES**,
escrita en C99. El repositorio no distribuye la ROM, gráficos, música ni datos
extraídos; cada usuario proporciona su copia legal.

## Novedades de v0.20

- La selección `MUSIC-1`, `MUSIC-2`, `MUSIC-3` y `OFF` ya tiene un cursor visible
  en las coordenadas usadas por el programa 6502 original.
- `tetris_apu_scenario` puede producir WAV además de trazas CSV.
- Los efectos pueden renderizarse con `--isolated`, sin música de fondo.
- `tools/build_audio_cache.py` genera localmente desde la ROM legal:
  - las diez pistas del driver original;
  - ocho efectos aislados;
  - alias OGG que usa el port durante la partida;
  - trazas por pista y efecto;
  - un manifiesto con SHA-256, CRC y hashes APU.
- El port continúa aceptando paquetes OGG por `--audio-pack`, la variable
  `TETRIS_AUDIO_PACK` o la carpeta `audio` de preferencias.
- CMake y el APK pasan a v0.20.

Los OGG generados nunca se incluyen en Git ni en los artefactos de Actions.

## Estado actual

- Modos A y B jugables a 60.0988 actualizaciones por segundo.
- Demo original desde comandos y piezas de la ROM.
- Menús, récords y finales reconstruidos desde PPU, CHR y OAM.
- Catedral B-Type animada mediante su máquina de estados original.
- Driver 6502/APU original ejecutable desde una ROM legal.
- Caché OGG local para evitar renderizar el APU dentro del callback durante la
  reproducción normal.
- Repeticiones deterministas y comparación de trazas.
- Teclado, controles táctiles y gamepad.
- Windows, Linux, ARM64 y ARMv7 sobre el mismo núcleo.

Todavía no es una decompilación bit a bit. Quedan diferencias de
microtemporización de bus, puntos de bucle exactos, validación APU contra Mesen,
diferencias de RAM/PPU de la demo y una construcción 6502 enlazable.

## Controles del menú GAME TYPE

```text
Izquierda / derecha : A-TYPE o B-TYPE
Abajo                : MUSIC-1 → MUSIC-2 → MUSIC-3 → OFF
Arriba               : recorrido inverso
Enter / Start        : selección de nivel
Retroceso / B        : volver
```

El cursor musical utiliza `X=$67` y `Y=$8F + musicType×$10`, igual que la rutina
original. En A-Type, la selección normal de nivel utiliza 0–9.

## Ejecutar el port de PC

```powershell
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes"
```

El título de la ventana debe mostrar `Tetris NES PC Port v0.20`.

## Generar todo el audio en OGG

Se necesita Python 3, ffmpeg y los ejecutables `tetris_apu_render` y
`tetris_apu_scenario` ya compilados.

```powershell
python tools\build_audio_cache.py `
  --rom "C:\ROMs\Tetris (USA).nes" `
  --bin-dir build\Release `
  --overwrite
```

En Linux:

```bash
python3 tools/build_audio_cache.py \
  --rom "$HOME/ROMs/Tetris (USA).nes" \
  --bin-dir build \
  --overwrite
```

La salida predeterminada coincide con la carpeta `audio` de preferencias que el
port busca automáticamente. También puede indicarse otra carpeta:

```powershell
python tools\build_audio_cache.py `
  --rom "C:\ROMs\Tetris (USA).nes" `
  --bin-dir build\Release `
  --output "C:\TetrisAudio" `
  --overwrite

.\tetris_pc.exe --rom "C:\ROMs\Tetris (USA).nes" --audio-pack "C:\TetrisAudio"
```

El paquete contiene `track_01.ogg` a `track_10.ogg`, los tres alias musicales
que usa actualmente el runtime, los ocho efectos y sus trazas. Consulta
[`docs/OGG_CACHE.md`](docs/OGG_CACHE.md).

## Generar una pista o efecto manualmente

```bash
tetris_apu_render "Tetris (USA).nes" 3 60 track-03.wav track-03.csv
tetris_apu_scenario "Tetris (USA).nes" rotate 240 effect-rotate.csv rotate.wav --isolated
```

Escenarios disponibles:

```text
move rotate lock line tetris level game-over complete
```

## Generar y comparar la matriz APU

```bash
python tools/apu_matrix.py capture \
  --apu-render ./tetris_apu_render \
  --apu-scenario ./tetris_apu_scenario \
  --rom "Tetris (USA).nes" \
  --output port-matrix

python tools/apu_matrix.py compare mesen-matrix port-matrix \
  --json apu-matrix-report.json
```

La herramienta espera 18 casos y compara ciclos, stalls, IRQ y escrituras
`$4000-$4017`. Las capturas se generan localmente con la ROM legal.

## Compilar y probar PC

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

La suite v0.20 añade la autoprueba del generador de caché OGG.

## Android

```bash
cd android
gradle --no-daemon :app:assembleDebug
```

El APK usa el mismo núcleo C99 y no incluye ROM, OGG, WAV ni trazas. La carga de
un caché OGG generado por el usuario en Android sigue siendo trabajo pendiente.

## Documentación

- [`docs/PORT_STATUS.md`](docs/PORT_STATUS.md)
- [`docs/OGG_CACHE.md`](docs/OGG_CACHE.md)
- [`docs/PLAYABILITY_VERIFICATION.md`](docs/PLAYABILITY_VERIFICATION.md)
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
ROM, PRG/CHR extraído, música de Nintendo, capturas de RAM, WAV u OGG generados,
ni tablas de audio extraídas.
