# Renderizado automático del APU original

La v0.11 incorpora una primera ruta automática que usa la ROM legal del
usuario para ejecutar el controlador de sonido 6502 de Tetris y producir audio
PCM/WAV. No incluye música, muestras ni tablas extraídas en el repositorio.

## Arquitectura

1. `cpu6502.c` ejecuta las instrucciones oficiales NMOS 6502 necesarias por el
   controlador de audio.
2. `rom_audio.c` mapea los 32 KiB de PRG en `$8000-$FFFF`, 2 KiB de RAM y los
   registros APU `$4000-$4017`.
3. Cada fotograma NTSC llama la entrada original `$E000`; la inicialización usa
   `$E006`.
4. `nes_apu.c` sintetiza dos pulsos, triángulo, ruido y DMC a 48 kHz.
5. `tetris_apu_render` escribe WAV mono PCM16 y opcionalmente una traza de
   escrituras APU por fotograma.

Los offsets solo se habilitan para la ROM comprobada CRC32 `D16EA396`.

## Generar un WAV

```bash
tetris_apu_render "Tetris (USA).nes" 1 60 music-1.wav
```

Parámetros:

- pista: `1` a `10`, tal como espera `musicTrack` en `$06F5`;
- duración: segundos, hasta una hora;
- salida: WAV mono, 48 kHz y 16 bits.

## Generar también una traza APU

```bash
tetris_apu_render "Tetris (USA).nes" 1 60 music-1.wav port-apu.csv
python tools/trace_compare.py mesen-apu.csv port-apu.csv --columns apu_writes
```

La columna `apu_writes` usa secuencias como
`4000=9F|4002=FD|4003=08`. La primera diferencia muestra el fotograma exacto en
el que el controlador embebido deja de coincidir con el emulador.

## Captura automática con Mesen

1. Abre la ROM legal en Mesen 2 o Mesen CE.
2. Abre la ventana Lua y permite acceso a I/O y funciones del sistema.
3. Ejecuta `tools/mesen_trace.lua`.
4. Inicia la demo o la escena que deseas comparar.

El script crea en la carpeta de datos Lua:

- `tetris-reference.csv`, con RAM de juego y escrituras APU por fotograma;
- `tetris-apu-writes.csv`, reducido a `frame,apu_writes`.

## Autopruebas sin ROM

```bash
tetris_apu_render --self-test
ctest --test-dir build -C Release --output-on-failure
```

La autoprueba usa un PRG artificial que escribe un tono en los registros de
pulso. No contiene bytes del juego.

## Límites actuales

- El núcleo APU es funcional y determinista, pero aún no es exacto por ciclo.
- El secuenciador de frames y los barridos se aproximan a nivel de muestra.
- La ruta DMC funciona, pero no modela robos de ciclos del CPU.
- El renderer automático se ofrece primero como herramienta WAV/traza; la
  sustitución del backend interactivo en todas las plataformas queda para la
  siguiente fase después de comparar sus registros contra Mesen.
- Las pistas y efectos se obtienen solo en ejecución desde la ROM aportada por
  el usuario.
