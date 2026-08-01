# Paquetes de audio OGG opcionales

La música de NES no está almacenada como WAV u OGG dentro de la ROM. El programa 6502 escribe notas,
volúmenes y efectos en el APU de la consola durante la ejecución. Por eso, para obtener audio idéntico
hay que ejecutar o traducir ese controlador y renderizar su salida.

La versión 0.5 puede cargar un **paquete Ogg Vorbis creado localmente por el usuario**. El repositorio y
los paquetes oficiales del port no incluyen grabaciones del cartucho.

## Archivos esperados

El directorio debe contener exactamente estos nombres:

```text
music_1.ogg
music_2.ogg
music_3.ogg
move.ogg
rotate.ogg
lock.ogg
line.ogg
tetris.ogg
level_up.ogg
game_over.ogg
complete.ogg
```

Las pistas de música se transmiten desde disco. Los efectos se decodifican como `Mix_Chunk`, por lo que
quedan preparados en memoria y no añaden retraso perceptible a los controles.

## Crear el paquete

Primero exporta o captura cada pista y efecto desde tu propia copia legal con un emulador o herramienta
APU. Nombra los archivos como en la lista anterior. Pueden ser WAV, FLAC, OGG, MP3 o M4A.

Con FFmpeg instalado:

```bash
python tools/audio_pack.py recordings audio
python tools/audio_pack.py --check audio
```

El conversor produce OGG a 48 kHz, un manifiesto con hashes y un aviso para no subir las grabaciones.
No recorta, normaliza ni modifica el tempo, de modo que conserva la captura suministrada.

## Dónde colocar el directorio

El juego busca, en este orden:

1. `--audio-pack RUTA` en la línea de comandos;
2. la variable de entorno `TETRIS_AUDIO_PACK`;
3. la carpeta `audio` dentro de las preferencias `YlPorts/TetrisNESPC`;
4. una carpeta `audio` junto al ejecutable;
5. `./audio` en el directorio actual.

Ejemplo:

```bash
./tetris_pc --rom "Tetris (USA).nes" --audio-pack ./audio
```

Si falta un archivo o la compilación no tiene SDL2_mixer, el juego conserva automáticamente el audio
sintetizado original del port. En pantalla, `OGG 1`, `OGG 2` o `OGG 3` indica que el paquete está activo.

## Siguiente paso técnico

Todavía falta traducir o encapsular el controlador APU original para que la conversión pueda hacerse
directamente desde la ROM legal, sin capturas manuales. El sistema de paquetes añadido aquí deja listo
el formato de salida y la reproducción de baja latencia para conectar ese renderizador más adelante.
