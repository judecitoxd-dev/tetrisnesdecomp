# Tetris NES — ports nativos para PC y Android

Versión **0.22** de una reimplementación portable de **Tetris (USA) para NES**
en C99. El repositorio no distribuye la ROM, gráficos, música ni datos
extraídos; cada usuario proporciona su copia legal.

## Novedades de v0.21

- La tabla de récords de la selección de nivel vuelve a mostrar el encabezado
  completo `NAME  SCORE  LV` y las tres filas dentro del marco original.
- Las posiciones se expresan mediante direcciones PPU verificables:
  `NAME=$224A`, `SCORE=$2250`, `LV=$2257` y filas `$2289/$22C9/$2309`.
- El callback de SDL ya no ejecuta el intérprete 6502 ni el APU.
- Un hilo productor de prioridad alta genera audio original por adelantado y lo
  deposita en un ring buffer SPSC de 32768 muestras.
- El callback se limita a copiar muestras preparadas, reduciendo música lenta,
  cortes y efectos trabados en PC y Android.
- El caché OGG generado desde la ROM carga las diez pistas originales y cambia
  entre las pistas normales 3–5 y allegro 6–8.
- El backend informa `ROM APU BUFFERED` o `OGG CACHE`, y registra underruns.
- CMake, paquetes y APK avanzan a v0.21.

## Estado actual

- Modos A y B jugables a 60.0988 actualizaciones por segundo.
- Menús, récords, juego y finales reconstruidos desde PRG, CHR, PPU y OAM.
- Música y efectos originales desde la ROM legal.
- Caché local Ogg Vorbis con diez pistas y ocho efectos aislados.
- Repeticiones deterministas, trazas APU y comparación con Mesen.
- Teclado, gamepad y controles táctiles.
- Windows, Linux, Android ARM64 y ARMv7 sobre el mismo núcleo.

La **correspondencia reproducible con la ROM** se estima en **45%**. Falta
aproximadamente **55%** para identidad funcional, audiovisual y de timing
respaldada por comparaciones. Es una estimación de ingeniería, no identidad
binaria ni cobertura automática.

## Ejecutar en PC

```powershell
.\tetris_pc.exe --rom "C:\ROMs\Tetris (USA).nes"
```

La consola debe indicar uno de estos backends:

```text
Audio backend: ROM APU BUFFERED
Audio backend: OGG CACHE
```

### Ajustar el margen del ring buffer

El objetivo predeterminado es 8192 muestras. En equipos lentos puede ampliarse:

```powershell
$env:TETRIS_AUDIO_RING_TARGET="16384"
.\tetris_pc.exe --rom "C:\ROMs\Tetris (USA).nes"
```

Se aceptan valores entre 2048 y 30720. Esto cambia la reserva de audio, no la
frecuencia ni el tono.

## Generar el caché OGG legal

Requiere `ffmpeg`, la ROM del usuario y los ejecutables de las herramientas:

```powershell
python tools\build_audio_cache.py `
  --rom "C:\ROMs\Tetris (USA).nes" `
  --bin-dir build\Release `
  --output "C:\TetrisAudio" `
  --overwrite

.\tetris_pc.exe `
  --rom "C:\ROMs\Tetris (USA).nes" `
  --audio-pack "C:\TetrisAudio"
```

El paquete contiene:

- `track_01.ogg` a `track_10.ogg`;
- movimiento, rotación, bloqueo, línea, Tetris, nivel, derrota y final;
- trazas CSV;
- `audio-cache.json` con SHA-256, CRC y hashes APU.

Los OGG, WAV y trazas generados permanecen fuera del repositorio.

## Compilar y probar PC

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

La suite v0.21 contiene 13 pruebas cuando Python está disponible, incluida la
geometría del ring buffer y la regresión de la tabla de récords.

## Android

```bash
cd android
gradle --no-daemon :app:assembleDebug
```

Android usa también el productor APU y el ring buffer. El APK no incluye ROM,
OGG, WAV ni trazas.

## Límites conocidos

- Todavía falta probar el audio de forma prolongada en hardware real y medir el
  contador de underruns en varios dispositivos.
- Los puntos de bucle de las pistas OGG son de duración fija; falta derivar la
  introducción y el bucle exactos del bytecode musical.
- Las escrituras APU aún no están modeladas al ciclo exacto de bus.
- Las 18 trazas necesitan referencias Mesen generadas localmente con la ROM.
- Parte de las reglas sigue implementada en C y no como una construcción 6502
  binariamente idéntica.

## Documentación

- `docs/PORT_STATUS.md`
- `docs/APU_RENDERING.md`
- `docs/OGG_CACHE.md`
- `docs/TRACE_COMPARISON.md`
- `docs/PLAYABILITY_VERIFICATION.md`
- `docs/ROM_SCREENS.md`
- `docs/ROM_MAP.md`
- `docs/ANDROID_PORT.md`

## Legalidad

El proyecto contiene código original, herramientas y documentación. No contiene
ROM, PRG/CHR extraído, música de Nintendo, capturas de RAM ni audio renderizado.


## Cambios de v0.22

- Los efectos del APU ya no descartan la música que estaba preparada en el
  ring buffer; mover o rotar una pieza no debe acelerar ni cortar la canción.
- El muestreo del APU ocurre durante todos los ciclos 6502, incluidos los
  ciclos del driver y los stalls DMC.
- PC conserva una ventana 4:3 exacta.
- Android usa orientación vertical con el juego arriba y controles grandes de
  estilo consola portátil abajo. Este diseño no se aplica al port de PC.
