# Tetris NES — ports nativos para PC y Android

Versión **0.11** de una reimplementación portable de **Tetris (USA) para NES**,
escrita en C99. El proyecto no distribuye la ROM, gráficos, música ni datos
extraídos: cada usuario proporciona su copia legal.

## Novedades de v0.11

- Captura automática de RAM y escrituras APU con `tools/mesen_trace.lua`.
- Intérprete 2A03/NMOS 6502 para las 151 instrucciones oficiales.
- Ejecución del controlador de sonido de la ROM mediante `$E006` al iniciar y
  `$E000` una vez por fotograma NTSC.
- Síntesis de los cinco canales del APU: dos pulsos, triángulo, ruido y DMC.
- Mezcla no lineal pulse/TND, envolventes, longitudes, barridos y filtro DC.
- Herramienta `tetris_apu_render` para crear WAV mono PCM16 a 48 kHz.
- Traza `frame,apu_writes` para comparar el driver del port con Mesen.
- Autopruebas con un PRG artificial que no contiene bytes del juego.

La ruta APU es funcional y automática, pero todavía no es exacta por ciclo. El
renderer WAV/traza se valida primero antes de sustituir el backend interactivo
de PC y Android.

## Estado actual

- Modos A y B jugables a 60.0988 actualizaciones por segundo.
- Demo original desde comandos y piezas de la ROM.
- Menús, récords y finales reconstruidos desde PPU, CHR y OAM.
- Teclado, controles táctiles y gamepad.
- Repeticiones deterministas y comparación de trazas por fotograma.
- Renderer automático del controlador musical/APU original desde la ROM legal.
- Audio sintetizado alternativo y paquetes OGG opcionales para el juego en vivo.

Todavía no es una decompilación bit a bit. Quedan la paridad APU por ciclo, la
integración del APU original como backend interactivo, diferencias de RAM/PPU,
parte de la catedral B-Type y una construcción 6502 enlazable.

## Generar audio original desde la ROM

```bash
tetris_apu_render "Tetris (USA).nes" 1 60 music-1.wav
```

También generar la traza de escrituras APU:

```bash
tetris_apu_render "Tetris (USA).nes" 1 60 music-1.wav port-apu.csv
python tools/trace_compare.py mesen-apu.csv port-apu.csv --columns apu_writes
```

Las pistas aceptadas son `1` a `10`. El renderer solo activa offsets originales
para la ROM verificada CRC32 `D16EA396`.

## Captura automática con Mesen

1. Abre tu ROM legal en Mesen 2 o Mesen CE.
2. Abre la ventana Lua y permite acceso a I/O y funciones del sistema.
3. Ejecuta `tools/mesen_trace.lua`.
4. Inicia la demo o escena que deseas comparar.

El script crea `tetris-reference.csv` y `tetris-apu-writes.csv` dentro de la
carpeta de datos del script.

## Ejecutar el port de PC

```powershell
.\tetris_pc.exe --rom "C:\ruta\Tetris (USA).nes"
```

## Compilar y probar

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

La aplicación Android sigue utilizando el backend interactivo existente. El
renderer APU original se integra primero como herramienta verificable y será el
backend en vivo después de cerrar diferencias contra Mesen.

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
