# Estado de decompilación y ports

## Resumen de v0.20

La fase v0.20 cambia la estrategia del audio interactivo. El driver 6502/APU se
conserva como fuente de verdad, pero puede ejecutarse fuera de la partida para
generar un caché Ogg Vorbis local. Durante el juego, SDL2_mixer reproduce las
muestras ya preparadas y evita que el callback de audio tenga que emular CPU y
APU en tiempo real.

También se corrige la falta de respuesta visual en `GAME TYPE`: v0.19 cambiaba
`music_track`, pero la pantalla exacta no dibujaba el cursor de `musicType`.
v0.20 localiza y decodifica `sprite53MusicTypeCursor` desde el PRG/CHR legal y
lo coloca con las coordenadas de la rutina 6502 original.

## Progreso estimado

| Área | Estado aproximado |
|---|---:|
| Port nativo jugable básico | 98% |
| Port Android jugable | 96% |
| Controles, tablero y puntuación | 98% |
| Modo A | 95% |
| Modo B | 95% |
| Integración y empaquetado | 99% |
| Carga legal de recursos desde ROM | 99% |
| Fidelidad de reglas y timings principales | 89% |
| Pantallas y animaciones originales/equivalentes | 96% |
| Audio original interactivo | 94% |
| Renderizado automático del APU original | 93% |
| Decompilación etiquetada/verificada del PRG 6502 | 44% |
| **Correspondencia reproducible con la ROM** | **42%** |

La cifra que representa mejor «ser 100% igual a la ROM» sigue siendo
**Correspondencia reproducible con la ROM**. El proyecto está aproximadamente
en **42%** y falta alrededor de **58%** para identidad funcional, audiovisual y
de timing respaldada por comparaciones reproducibles.

Estos valores son estimaciones de ingeniería, no cobertura automática de
líneas. Actions no contiene una ROM y no puede validar por sí solo la salida
visual o auditiva final de una partida humana.

## Caché de audio legal

`tools/build_audio_cache.py` genera desde la ROM del usuario:

- `track_01.ogg` a `track_10.ogg`;
- tres alias de música que consume el runtime actual;
- movimiento, rotación, bloqueo, línea, Tetris, subida de nivel, derrota y
  final completado;
- una traza CSV por pista y efecto;
- `audio-cache.json` con SHA-256, CRC, duración, muestras y hashes APU.

Los efectos utilizan el nuevo modo `--isolated` de `tetris_apu_scenario`, por lo
que no contienen MUSIC-1 debajo. El repositorio y los artefactos CI excluyen los
OGG, WAV, CSV y la ROM.

## Selección musical verificada

La rutina original usa `musicType` con valores 0–3:

- 0: MUSIC-1;
- 1: MUSIC-2;
- 2: MUSIC-3;
- 3: OFF.

El cursor se coloca en:

- X NES: `$67`;
- Y NES: `$8F + musicType × $10`.

El metasprite encontrado en el PRG contiene dos tiles `$27`; el segundo usa
reflejo horizontal y desplazamiento X `$4A`. El renderer usa esos datos y los
graba con el banco CHR y la paleta de la ROM, en vez de dibujar una fuente o un
rectángulo inventado.

La transición de estado es compartida por teclado, mando y controles táctiles.
El cursor permite comprobar visualmente que Up/Down modifican la fila.

## Correspondencia de pistas

El port mantiene la numeración del driver original, 1–10. Las selecciones del
menú que usa actualmente el juego corresponden a:

- MUSIC-1: pista 3;
- MUSIC-2: pista 4;
- MUSIC-3: pista 5;
- variantes allegro: pistas 6, 7 y 8.

El generador conserva todas las pistas. El runtime v0.20 todavía carga de forma
directa los tres alias normales y los ocho efectos; integrar las diez pistas,
las variantes allegro y los finales mediante el manifiesto es la siguiente
fase.

## Direcciones y timings comprobados

- `LINES`: `$2073`
- `TOP`: `$20B8`
- `SCORE`: `$2118`
- `LEVEL`: `$22BA`
- Estadísticas: `$2186`, `$21C6`, `$2206`, `$2246`, `$2286`, `$22C6`, `$2306`
- Filas de récords: `$2289`, `$22C9`, `$2309`
- `musicType`: `$00C2`
- `musicTrack`: `$06F5`
- Cursor de música: X `$67`, base Y `$8F`, paso `$10`
- Gravedad de nivel 0: 48 fotogramas NTSC
- Frecuencia lógica: 60.0988 actualizaciones por segundo

## Suite reproducible

Cuando Python está disponible, la suite v0.20 contiene 12 pruebas:

1. reglas y estados de juego;
2. demo desde ROM;
3. catedral B-Type;
4. APU;
5. timing IRQ;
6. marco de llamadas 6502;
7. récords y migración;
8. regresiones de jugabilidad;
9. autorender APU;
10. escenarios APU y salida WAV;
11. matriz APU;
12. planificación y manifiesto del caché OGG.

La autoprueba del caché confirma diez pistas, ocho efectos, tres alias y 22
archivos obligatorios sin necesitar una ROM.

## Diferencias conocidas

- El caché requiere una ejecución local con ROM y ffmpeg; CI solo valida su
  planificación y las herramientas.
- Las pistas OGG tienen duración fija. Falta detectar el punto de bucle exacto
  del bytecode musical y reproducir introducción/bucle sin corte.
- Android todavía usa el backend APU; falta integrar SDL2_mixer o un decodificador
  OGG equivalente en el APK.
- Las escrituras APU se aplican a granularidad de instrucción y no al ciclo de
  bus exacto.
- La matriz necesita capturas Mesen generadas localmente para medir igualdad.
- La demo usa comandos y piezas originales, pero parte de sus reglas está en C.
- No existe todavía una construcción 6502 binariamente idéntica.

## Próxima fase hacia exactitud

1. Cargar las diez pistas del manifiesto y cambiar a las pistas 6–8 al entrar en
   allegro.
2. Detectar automáticamente los puntos de bucle del estado musical 6502.
3. Integrar el caché OGG generado por el usuario en Android.
4. Comparar las 18 trazas con Mesen y corregir la primera divergencia.
5. Continuar etiquetando y traduciendo rutinas PRG.
6. Preparar una construcción 6502 enlazable y perseguir identidad binaria.
