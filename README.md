# Tetris NES — ports nativos para PC y Android

Versión **0.22** de una reimplementación portable de **Tetris (USA) para NES**
en C99. El repositorio no distribuye la ROM, gráficos, música ni datos
extraídos; cada usuario proporciona su copia legal.

## Novedades de v0.22

- El APU se muestrea durante todos los ciclos del 6502: instrucciones del
  driver, tiempo libre del fotograma y stalls DMC.
- Los efectos ya no vacían la música preparada en el ring buffer. Mover,
  rotar o bloquear una pieza no debe adelantar ni cortar la canción.
- El callback de SDL continúa limitado a copiar muestras ya preparadas por el
  hilo productor.
- La reserva normal baja a 4096 muestras para reducir la latencia sin alterar
  tempo ni afinación.
- PC conserva la presentación clásica en una ventana 4:3 exacta.
- **Solo Android** usa orientación vertical tipo consola portátil: juego grande
  arriba y controles táctiles más grandes debajo.
- La tabla de récords conserva `NAME  SCORE  LV` y sus tres filas dentro del
  marco original.
- CMake, paquetes, título de ventana y APK avanzan a v0.22.

## Estado actual

- Modos A y B jugables a 60.0988 actualizaciones por segundo.
- Menús, récords, juego y finales reconstruidos desde PRG, CHR, PPU y OAM.
- Música y efectos originales desde la ROM legal.
- Caché local Ogg Vorbis con diez pistas y ocho efectos aislados.
- Repeticiones deterministas, trazas APU y comparación con Mesen.
- Teclado, gamepad y controles táctiles.
- Windows, Linux, Android ARM64 y ARMv7 sobre el mismo núcleo.

La **correspondencia reproducible con la ROM** se estima en **46%**. Falta
aproximadamente **54%** para identidad funcional, audiovisual y de timing
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

El objetivo predeterminado es **4096 muestras**. En equipos que presenten
underruns puede ampliarse:

```powershell
$env:TETRIS_AUDIO_RING_TARGET="8192"
.\tetris_pc.exe --rom "C:\ROMs\Tetris (USA).nes"
```

Se aceptan valores entre 2048 y 30720. Esto cambia la reserva y la latencia,
pero no debe cambiar la frecuencia, el tono ni el tempo.

## Presentación por plataforma

### PC

La ventana se normaliza a una relación **4:3**. El juego conserva la interfaz
clásica; no recibe la carcasa ni los botones tipo consola portátil.

### Android

Android se bloquea en orientación vertical. El lienzo lógico es de 640×1280:
la imagen del juego ocupa la zona superior de 640×480 y la zona inferior
contiene una cruceta grande, botones A/B/DROP, START/SELECT y accesos de ROM y
edición. Este diseño es exclusivo de Android.

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

La suite v0.22 contiene 13 pruebas cuando Python está disponible, además de una
verificación de fuente que comprueba la continuidad musical, el diseño Android
vertical y la normalización 4:3 de PC.

## Android

```bash
cd android
gradle --no-daemon :app:assembleDebug
```

Android usa también el productor APU y el ring buffer. El APK no incluye ROM,
OGG, WAV ni trazas.

## Límites conocidos

- Actions puede verificar conteos de ciclos, número de muestras y continuidad
  de la cola, pero no puede escuchar el resultado en el equipo del usuario.
- Todavía falta medir el contador de underruns durante partidas prolongadas en
  varios dispositivos.
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
