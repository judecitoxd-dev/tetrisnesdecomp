# Estado de decompilación y ports

## Resumen de v0.22

La fase v0.22 responde a la prueba auditiva de v0.21. El ring buffer evitó
trabajo pesado dentro del callback, pero cada efecto todavía vaciaba la música
preparada. Como el productor ya había avanzado el driver, cada movimiento,
rotación o bloqueo saltaba una porción de la canción y producía una sensación
de tempo acelerado y audio cortado.

v0.22 mantiene la cola musical al insertar efectos y cambia el renderer a una
línea temporal continua: las muestras se capturan durante las instrucciones del
driver 6502, el tiempo libre del fotograma y los stalls DMC.

La presentación también se separa por plataforma. PC conserva una ventana 4:3;
solo Android adopta orientación vertical con el juego arriba y controles tipo
consola portátil debajo.

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
| Fidelidad de reglas y timings principales | 92% |
| Pantallas y animaciones originales/equivalentes | 97% |
| Audio original interactivo | 97% |
| Renderizado automático del APU original | 95% |
| Decompilación etiquetada/verificada del PRG 6502 | 52% |
| **Correspondencia reproducible con la ROM** | **50%** |

La cifra central es **Correspondencia reproducible con la ROM**: 50% terminado
y aproximadamente 50% pendiente para identidad funcional, audiovisual y de
timing. Los porcentajes son estimaciones de ingeniería, no cobertura automática
ni una afirmación de identidad binaria.

El punto adicional de correspondencia refleja una condición reproducible: los
eventos de juego ya no descartan muestras musicales y todos los ciclos físicos
del fotograma participan en el reloj de muestreo. La afinación final todavía
debe confirmarse escuchando el artefacto con la ROM legal.

## Línea temporal continua del APU

El reloj de salida relaciona 48000 muestras por segundo con el reloj NTSC de
1789773 ciclos por segundo mediante un acumulador entero. Cada ciclo puede
producir una muestra en su posición temporal real.

Esto incluye:

1. instrucciones ejecutadas por `updateAudio`;
2. ciclos libres hasta completar el fotograma NTSC;
3. stalls provocados por el DMC;
4. cambios de registros APU realizados por el driver original.

El callback SDL sigue limitado a leer el ring buffer. No ejecuta el intérprete
6502 ni el mezclador del APU.

## Continuidad de música y efectos

En v0.21, `tetris_audio_play_events()` solicitaba un vaciado de la cola para
cada efecto. El productor continuaba desde su estado futuro, por lo que las
muestras descartadas nunca volvían a reproducirse.

En v0.22:

- movimiento, rotación, bloqueo, línea y demás efectos conservan la cola;
- un cambio real de pista o de modo normal/allegro sí puede resincronizarla;
- el objetivo normal baja de 8192 a 4096 muestras para reducir latencia;
- `TETRIS_AUDIO_RING_TARGET` sigue disponible para diagnosticar underruns;
- modificar ese valor no debe alterar tono ni tempo.

## Presentación por plataforma

### PC

- Ventana clásica sin carcasa ni controles táctiles tipo Game Boy.
- Tamaño persistido normalizado a una relación 4:3 exacta.
- El redimensionado mantiene la misma relación.

### Android

- Orientación vertical obligatoria.
- Lienzo lógico 640×1280.
- Juego en la región superior 640×480.
- Cruceta y botones grandes en la región inferior.
- Botones A/B/DROP diferenciados y accesos START/SELECT/ROM/EDIT.
- El diseño portátil se compila únicamente bajo `__ANDROID__`.

## Tabla de récords

Se conserva la reparación de v0.21 mediante estas direcciones PPU:

- `NAME`: `$224A`
- `SCORE`: `$2250`
- `LV`: `$2257`
- fila 1: `$2289`
- fila 2: `$22C9`
- fila 3: `$2309`

El paso de presentación limpia únicamente las celdas de texto, conserva el
marco amarillo y vuelve a dibujar el encabezado completo y las tres filas.

## Caché OGG

El runtime carga las diez pistas generadas:

- `track_01.ogg` a `track_10.ogg`;
- música normal del menú: pistas 3, 4 y 5;
- variantes allegro: pistas 6, 7 y 8;
- ocho efectos aislados.

El caché continúa siendo local y derivado de la ROM del usuario. Ningún OGG,
WAV, CSV o archivo de ROM se publica en Git o en el APK.

## Suite reproducible

Cuando Python está disponible, v0.22 contiene 13 pruebas C/CMake y una
verificación de fuente adicional:

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
13. planificación del caché OGG;
14. continuidad musical, Android vertical y PC 4:3.

## Diferencias conocidas

- Las pruebas automáticas no escuchan el audio en el equipo del usuario.
- Falta confirmar que el contador de underruns permanezca en cero durante una
  partida prolongada en PC y Android.
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
3. Detectar puntos de introducción y bucle exactos para las diez pistas.
4. Sustituir más lógica C por rutinas etiquetadas del PRG.
5. Ampliar las pruebas visuales a capturas comparadas píxel por píxel.
6. Preparar una construcción 6502 enlazable y perseguir identidad binaria.

## Evidencia añadida en v0.23

- 53 firmas de rutinas enlazadas a nombres semánticos.
- 9 tablas completas y 14 llamadas directas verificadas.
- 1,136 bytes comprobados por firmas o hashes de tablas.
- Hash exacto del PRG completo, sin contabilizarlo falsamente como decompilado.
- Regresiones de DAS, prioridad simultánea, pared, ARE, línea y cortina.

Las cifras suben solo por evidencia reproducible. Todavía falta traducir y
comparar más estados del bucle principal, menús, PPU, puntuación BCD y finales.
