# Tetris NES — ports nativos para PC y Android

Versión **0.19** de una reimplementación portable de **Tetris (USA) para NES**,
escrita en C99. El repositorio no distribuye la ROM, gráficos, música ni datos
extraídos; cada usuario proporciona su copia legal.

## Reparación de jugabilidad de v0.19

Esta versión corrige regresiones introducidas mientras se reconstruían las
pantallas originales:

- El limpiado de estadísticas ya no borra la primera columna del tablero ni
  hace desaparecer partes de las piezas.
- Los ajustes antiguos que guardaron niveles ocultos 10–19 se migran al dígito
  visible del menú original (`10 → 0`, `18 → 8`, `19 → 9`).
- La selección normal vuelve a estar limitada a los niveles originales 0–9.
- En `GAME TYPE`, izquierda/derecha eligen A-Type o B-Type y arriba/abajo
  recorren `MUSIC-1`, `MUSIC-2`, `MUSIC-3` y `OFF`.
- Teclado, mando y botones táctiles utilizan la misma lógica de navegación.
- El audio APU solicita un búfer más amplio para reducir cortes mientras el
  driver 6502 original se ejecuta en tiempo real.
- El título del ejecutable, CMake, APK, documentación y paquetes pasan a v0.19.

Las regresiones anteriores tienen una prueba específica llamada
`playability_regressions`; no se consideran resueltas únicamente porque el
programa compile.

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

Todavía no es una decompilación bit a bit. Quedan diferencias de
microtemporización de bus, validación APU contra capturas Mesen reales,
diferencias de RAM/PPU de la demo y una construcción 6502 enlazable.

## Controles de los menús originales

En la pantalla `GAME TYPE`:

```text
Izquierda / derecha : A-TYPE o B-TYPE
Abajo                : MUSIC-1 → MUSIC-2 → MUSIC-3 → OFF
Arriba               : recorrido inverso
Enter / Start        : selección de nivel
Retroceso / B        : volver
```

En A-Type, la selección normal de nivel utiliza 0–9. Los niveles superiores se
alcanzan durante la partida según las reglas del juego; no se guardan como una
selección invisible del menú.

## Ejecutar el port de PC

```powershell
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes"
```

El título de la ventana debe mostrar `Tetris NES PC Port v0.19`.

## Ajustar el margen del audio

v0.19 solicita 2048 muestras en PC y 4096 en Android. Para probar otro tamaño:

```powershell
$env:TETRIS_AUDIO_BUFFER_SAMPLES="4096"
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes"
```

Se aceptan potencias de dos entre 256 y 8192. Esto reduce underruns, pero no se
presenta como una solución definitiva: el siguiente paso es separar el
renderizado APU del callback mediante un productor y un búfer circular.

## Generar una pista o efecto original

```bash
tetris_apu_render "Tetris (USA).nes" 1 60 music-1.wav track-01.csv
tetris_apu_scenario "Tetris (USA).nes" rotate 180 effect-rotate.csv
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
`$4000-$4017`. Las capturas de referencia se generan localmente con la ROM
legal y permanecen fuera del repositorio.

## Compilar y probar PC

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

La suite v0.19 contiene 11 pruebas, incluida la regresión de jugabilidad.

## Android

```bash
cd android
gradle --no-daemon :app:assembleDebug
```

El APK usa el mismo núcleo C99 y no incluye ROM, audio renderizado ni trazas.

## Documentación

- [`docs/PORT_STATUS.md`](docs/PORT_STATUS.md)
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
ROM, PRG/CHR extraído, música de Nintendo, capturas de RAM, WAV generados ni
tablas de audio extraídas.
