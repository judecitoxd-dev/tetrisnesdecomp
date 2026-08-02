# Estado de decompilación y ports

## Resumen de v0.19

La fase v0.19 es una reparación de jugabilidad y de correspondencia con el
renderizador original. No se limita a cambiar posiciones visuales: identifica
las causas de una columna borrada, el inicio accidental en nivel 10, la
navegación incompleta del menú musical y los cortes producidos por un callback
de audio con poco margen.

La primera columna del tablero desaparecía porque una restauración del fondo de
estadísticas alcanzaba el tile X=12, exactamente donde empieza el playfield. El
nivel 10 provenía de ajustes antiguos que guardaban 10–19 aunque el menú visible
solo contiene 0–9. La navegación de `GAME TYPE` trataba todas las flechas como
cambio A/B y no reproducía el estado `musicType` del programa 6502.

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
| **Fidelidad de reglas y timings principales** | **89%** |
| Pantallas y animaciones originales/equivalentes | 96% |
| Audio original interactivo | 93% |
| Renderizado automático del APU original | 92% |
| **Decompilación etiquetada/verificada del PRG 6502** | **43%** |
| **Correspondencia reproducible con la ROM** | **40%** |

Estos porcentajes son estimaciones de ingeniería, no cobertura automática de
líneas. Los tres aumentos resaltados se justifican así:

- Reglas/timings: navegación original de tipo/música, selección normal 0–9,
  inicio comprobado en nivel 0 y gravedad de 48 fotogramas.
- PRG 6502: etiquetado y uso verificable de las rutas de menú y direcciones PPU
  del render original.
- Correspondencia: constantes compartidas, límites de nametable y pruebas que
  fallan si una restauración vuelve a invadir el playfield.

Los porcentajes generales de jugabilidad se mantienen por debajo de 100% hasta
que los artefactos se prueben de forma interactiva con una ROM legal. GitHub
Actions no incorpora la ROM y, por tanto, no puede validar por sí solo la imagen
o el sonido final durante una partida humana.

## Reparaciones verificables de v0.19

### Límite entre estadísticas y tablero

- El tablero exacto empieza en tile X=12, Y=6 y mide 10×20 tiles.
- La restauración antigua cubría X=4..12 e incluía la primera columna.
- La nueva región termina antes de X=12.
- `playability_regressions` comprueba esta desigualdad en cada build.

### Nivel inicial y ajustes antiguos

- El selector normal utiliza únicamente niveles 0–9.
- Ajustes v0.18 con niveles ocultos se migran por el dígito que mostraban:
  `10 → 0`, `18 → 8`, `19 → 9`.
- Los replays históricos todavía pueden describir niveles superiores; no se
  cambia su formato ni su reproducción determinista.

### Navegación de GAME TYPE / MUSIC TYPE

- Izquierda/derecha: A-Type o B-Type.
- Abajo: Music 1 → Music 2 → Music 3 → Off.
- Arriba: recorrido inverso.
- Teclado y mando llaman la misma función de transición.
- Los botones táctiles generan las mismas teclas y heredan esa lógica.

### Audio

El driver original 6502/APU todavía se ejecuta en el callback de SDL. v0.19
amplía el búfer solicitado de 512 a 2048 muestras en PC y 4096 en Android,
configurable mediante `TETRIS_AUDIO_BUFFER_SAMPLES`.

Esto reduce el riesgo de underruns, pero no cierra el audio al 100%. Falta mover
el renderizado a un productor con búfer circular, medir underruns y comparar las
18 trazas contra capturas reales de Mesen.

## Direcciones y timings comprobados

- `LINES`: `$2073`
- `TOP`: `$20B8`
- `SCORE`: `$2118`
- `LEVEL`: `$22BA`
- Estadísticas: `$2186`, `$21C6`, `$2206`, `$2246`, `$2286`, `$22C6`, `$2306`
- Filas de récords: `$2289`, `$22C9`, `$2309`
- Gravedad de nivel 0: 48 fotogramas NTSC.
- Frecuencia lógica del port: 60.0988 actualizaciones por segundo.

## Suite reproducible

La suite v0.19 contiene 11 pruebas:

1. reglas y estados de juego;
2. demo desde ROM;
3. catedral B-Type;
4. APU;
5. timing IRQ;
6. marco de llamadas 6502;
7. récords y migración;
8. regresiones de jugabilidad;
9. autorender APU;
10. escenarios APU;
11. matriz APU.

La prueba de jugabilidad comprueba explícitamente el límite del tablero, la
migración del nivel 10, el rango 0–9, el orden musical y el timing de nivel 0.

## Diferencias conocidas

- La estabilidad del audio necesita una prueba interactiva prolongada y un
  productor/ring buffer; ampliar el callback no demuestra ausencia total de
  cortes.
- Las escrituras APU siguen aplicándose a granularidad de instrucción y no al
  ciclo exacto de bus.
- La matriz necesita capturas Mesen generadas localmente para medir igualdad.
- La demo usa comandos y piezas originales, pero parte de sus reglas se ejecuta
  en C.
- Aún no existe una construcción 6502 enlazable o binariamente idéntica.
- La fidelidad visual final requiere comparar capturas del artefacto con la ROM
  legal del usuario.

## Próxima fase hacia exactitud

1. Ejecutar el artefacto v0.19 con la ROM legal y comparar capturas de partida,
   nivel, menús, récords y finales.
2. Sustituir el render APU dentro del callback por productor y búfer circular.
3. Capturar la matriz Mesen y corregir la primera divergencia de cada familia.
4. Ampliar las pruebas de input a secuencias completas de menús y partida.
5. Continuar etiquetando y traduciendo rutinas PRG.
6. Preparar una construcción 6502 enlazable y perseguir identidad binaria.
