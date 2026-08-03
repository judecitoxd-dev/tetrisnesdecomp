# Estado de decompilación y ports

## Resumen de v0.21

La fase v0.21 responde a dos fallos confirmados durante una prueba interactiva:
la cabecera de récords de la selección de nivel aparecía cortada y el audio
original seguía ejecutando el 6502/APU dentro del callback de SDL.

La tabla se repara mediante sus direcciones PPU, y el audio se mueve a un hilo
productor que mantiene un ring buffer. El callback de tiempo real solo copia
muestras ya preparadas.

## Progreso estimado

| Área | Estado aproximado |
|---|---:|
| Port nativo jugable básico | 98% |
| Port Android jugable | 97% |
| Controles, tablero y puntuación | 98% |
| Modo A | 95% |
| Modo B | 95% |
| Integración y empaquetado | 99% |
| Carga legal de recursos desde ROM | 99% |
| Fidelidad de reglas y timings principales | 89% |
| Pantallas y animaciones originales/equivalentes | 97% |
| Audio original interactivo | 96% |
| Renderizado automático del APU original | 94% |
| Decompilación etiquetada/verificada del PRG 6502 | 45% |
| **Correspondencia reproducible con la ROM** | **45%** |

La cifra central es **Correspondencia reproducible con la ROM**: 45% terminado
y aproximadamente 55% pendiente para identidad funcional, audiovisual y de
timing. Los porcentajes son estimaciones de ingeniería, no cobertura automática
ni una afirmación de identidad binaria.

## Reparación de la tabla de récords

La captura recibida mostraba `NAME SCO`, porque el overlay anterior dependía de
parte del encabezado estático y no reconstruía todos los campos al final del
frame.

v0.21 usa estas direcciones:

- `NAME`: `$224A`
- `SCORE`: `$2250`
- `LV`: `$2257`
- fila 1: `$2289`
- fila 2: `$22C9`
- fila 3: `$2309`

El nuevo paso de presentación limpia únicamente las celdas de texto y conserva
el marco amarillo de la ROM. Después dibuja el encabezado completo y las tres
filas, impidiendo que `SCORE` o `LV` crucen el borde derecho de la nametable.

## Audio original con productor y ring buffer

La implementación anterior llamaba `tetris_rom_audio_run_frame()` desde el
callback de SDL. Si el intérprete 6502 y el APU tardaban más que el margen del
dispositivo, la salida se escuchaba lenta, cortada o trabada.

v0.21 separa responsabilidades:

1. Un hilo productor de prioridad alta ejecuta el driver original.
2. Los frames se almacenan en un ring buffer SPSC de 32768 muestras.
3. El callback consume muestras preparadas sin emular CPU/APU.
4. Los cambios de pista, allegro y efectos descartan audio obsoleto.
5. Los underruns se cuentan para futuras pruebas en hardware real.

El objetivo normal es 8192 muestras y puede ajustarse con
`TETRIS_AUDIO_RING_TARGET`. Esto no cambia la frecuencia ni el tono.

## Caché OGG

El runtime carga ahora las diez pistas generadas:

- `track_01.ogg` a `track_10.ogg`;
- música normal del menú: pistas 3, 4 y 5;
- variantes allegro: pistas 6, 7 y 8;
- ocho efectos aislados.

El caché continúa siendo local y derivado de la ROM del usuario. Ningún OGG,
WAV, CSV o archivo de ROM se publica en Git o en el APK.

## Suite reproducible

Cuando Python está disponible, v0.21 contiene 13 pruebas:

1. reglas y estados de juego;
2. demo desde ROM;
3. catedral B-Type;
4. APU;
5. timing IRQ;
6. marcos de llamada 6502;
7. récords y persistencia;
8. regresiones visuales y de menú;
9. geometría y wrap del ring buffer;
10. autorender APU;
11. escenarios APU;
12. matriz APU;
13. planificación del caché OGG.

## Diferencias conocidas

- Las pruebas automáticas no escuchan el audio en el equipo del usuario; una
  prueba prolongada debe confirmar que el contador de underruns permanece en
  cero o cerca de cero.
- Los OGG aún usan duración fija; falta detectar introducción y punto de bucle
  exactos del bytecode musical.
- Las escrituras APU se aplican a granularidad de instrucción, no al ciclo
  exacto de bus.
- Las 18 trazas requieren capturas Mesen reales para corregir la primera
  divergencia de cada familia.
- Parte de las reglas y pantallas sigue implementada en C.
- Todavía no existe una construcción 6502 enlazable y binariamente idéntica.

## Próxima fase hacia exactitud

1. Exponer el contador de underruns en una pantalla de diagnóstico.
2. Comparar las 18 trazas con capturas Mesen.
3. Detectar puntos de bucle exactos para las diez pistas.
4. Sustituir más lógica C por rutinas etiquetadas del PRG.
5. Ampliar las pruebas visuales a capturas comparadas píxel por píxel.
6. Preparar una construcción 6502 enlazable y perseguir identidad binaria.
