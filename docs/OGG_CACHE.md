# Caché OGG generado desde la ROM legal

La v0.20 añade un camino para evitar que el emulador 6502/APU tenga que generar
muestras dentro del callback de SDL. Las pistas y efectos se renderizan una sola
vez desde la ROM legal del usuario, se convierten localmente a Ogg Vorbis y el
port los reproduce mediante SDL2_mixer.

El repositorio no contiene ROM, WAV, OGG, tablas musicales ni audio extraído.
La carpeta generada es privada del usuario y no debe subirse al repositorio.

## Contenido generado

`tools/build_audio_cache.py` crea:

- `track_01.ogg` a `track_10.ogg`: las diez selecciones del driver original;
- `music_1.ogg`, `music_2.ogg`, `music_3.ogg`: alias que usa actualmente el
  selector normal del port;
- `move.ogg`;
- `rotate.ogg`;
- `lock.ogg`;
- `line.ogg`;
- `tetris.ogg`;
- `level_up.ogg`;
- `game_over.ogg`;
- `complete.ogg`;
- una traza CSV para cada pista y efecto;
- `audio-cache.json` con SHA-256, CRC reportado por el renderer, parámetros y
  hashes APU.

Los efectos se renderizan con `--isolated`: no incluyen música de fondo.

## Requisitos

1. Compilar `tetris_apu_render` y `tetris_apu_scenario`.
2. Tener Python 3.
3. Tener `ffmpeg` con el encoder `libvorbis`.
4. Proporcionar una copia legal compatible de `Tetris (USA).nes`.

## Windows

Desde la carpeta del proyecto compilado:

```powershell
python tools\build_audio_cache.py `
  --rom "C:\ROMs\Tetris (USA).nes" `
  --bin-dir build\Release `
  --overwrite
```

La salida predeterminada coincide con la carpeta `audio` de preferencias que el
port busca automáticamente. También puede elegirse otra carpeta:

```powershell
python tools\build_audio_cache.py `
  --rom "C:\ROMs\Tetris (USA).nes" `
  --bin-dir build\Release `
  --output "C:\TetrisAudio" `
  --overwrite

.\tetris_pc.exe --rom "C:\ROMs\Tetris (USA).nes" --audio-pack "C:\TetrisAudio"
```

## Linux

```bash
python3 tools/build_audio_cache.py \
  --rom "$HOME/ROMs/Tetris (USA).nes" \
  --bin-dir build \
  --overwrite
```

## Parámetros

- `--music-seconds 180`: duración de cada pista. Puede aumentarse hasta 3600.
- `--effect-frames 240`: duración capturada para cada efecto.
- `--quality 5`: calidad Vorbis entre 0 y 10.
- `--keep-wav`: conserva los WAV temporales.
- `--output`: carpeta de salida.
- `--ffmpeg`: ruta explícita al ejecutable.
- `--overwrite`: permite regenerar un paquete existente.

## Limitaciones actuales

El caché elimina el trabajo pesado del APU durante la reproducción, pero la
correspondencia aún no es 100%:

- el runtime v0.20 reproduce directamente los tres alias seleccionables y los
  ocho efectos; conserva las otras siete pistas para la siguiente fase;
- las pistas tienen una duración fija y SDL2_mixer vuelve al inicio del archivo;
  todavía falta detectar el punto de bucle exacto del driver y guardar sus
  muestras de inicio/bucle por separado;
- las trazas deben compararse contra Mesen para corregir divergencias del APU;
- Android necesita incorporar el mismo cargador OGG en su paquete nativo.

Por estas razones, generar OGG soluciona los cortes de tiempo real, pero no se
presenta como identidad total con la ROM.
