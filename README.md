# Tetris NES — ports nativos para PC y Android

Versión **0.12** de una reimplementación portable de **Tetris (USA) para NES**,
escrita en C99. El proyecto no distribuye la ROM, gráficos, música ni datos
extraídos: cada usuario proporciona su copia legal.

## Novedades de v0.12

- El controlador de sonido 6502 original funciona ahora como backend en vivo.
- Música y efectos se generan desde la ROM durante partida, demo y replay.
- Windows, Linux y Android comparten el mismo CPU 6502, APU y driver.
- Las tres selecciones musicales usan los comandos originales 3, 4 y 5.
- Cambio automático a las variantes allegro 6, 7 y 8 al subir el tablero.
- Movimiento, rotación, bloqueo, línea, Tetris, nivel, derrota y final usan los
  slots originales del controlador.
- Al cambiar la ROM se desconecta el driver anterior antes de liberar su PRG.
- OGG sigue disponible como opción y el sintetizador incorporado funciona como
  respaldo cuando la ROM no tiene offsets verificados.
- Las herramientas WAV, trazas Mesen y autopruebas de v0.11 continúan incluidas.

La integración interactiva está terminada, pero la emulación APU todavía no es
exacta por ciclo. Quedan la temporización de cada instrucción, IRQ y robos de
ciclos del DMC.

## Estado actual

- Modos A y B jugables a 60.0988 actualizaciones por segundo.
- Demo original desde comandos y piezas de la ROM.
- Menús, récords y finales reconstruidos desde PPU, CHR y OAM.
- Teclado, controles táctiles y gamepad.
- Repeticiones deterministas y comparación de trazas por fotograma.
- Música y efectos originales interactivos desde la ROM legal.
- Renderer WAV y traza APU automática para análisis offline.
- Paquetes OGG opcionales y sintetizador de respaldo.

Todavía no es una decompilación bit a bit. Quedan la paridad APU por ciclo,
diferencias de RAM/PPU, parte del movimiento de la catedral B-Type y una
construcción 6502 enlazable.

## Ejecutar el port de PC

```powershell
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes"
```

Cuando la ROM coincide con CRC32 `D16EA396`, la consola informa:

```text
Audio backend: ROM APU
```

## Generar audio original desde la ROM

```bash
tetris_apu_render "Tetris (USA).nes" 1 60 music-1.wav
```

También generar la traza de escrituras APU:

```bash
tetris_apu_render "Tetris (USA).nes" 1 60 music-1.wav port-apu.csv
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

Autoprueba directa del renderer:

```bash
tetris_apu_render --self-test
```

## Android

```bash
cd android
gradle --no-daemon :app:assembleDebug
```

El APK compila el intérprete 6502, el núcleo APU y el driver original para
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
