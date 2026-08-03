# Estado de implementación GBC — v0.4

## Completado

- Cartucho CGB-only de 32 KiB y conversor Windows GUI/CLI.
- Validación de Tetris (USA), CRC32 `D16EA396`.
- Interfaz compacta 160×144 con las 20 filas del tablero visibles.
- Banco VRAM 1 con 256 combinaciones de cuatro celdas 4×4 por tile.
- Mini-piezas derivadas de los bloques NES originales.
- Paneles simultáneos: tipo, líneas, estadísticas, top, score, next y nivel.
- Contadores de aparición para T/J/Z/O/S/L/I.
- Título, A/B y selección de nivel/altura completos y sin paneo.
- A-Type, B-Type, basura por altura, 25 líneas y estado completado.
- Rotaciones, colisiones, soft drop, lock, líneas, score, nivel y paletas.
- Redibujado compatible con restricciones reales de VRAM.
- Pruebas LR35902 de menús, tablero compacto, lock, score y B-Type.

## Pendiente para paridad completa

1. RNG de dos bytes y selección one-reroll exacta.
2. Lock pending, row check, animación de línea y ARE exactos.
3. High scores, entrada de nombre y persistencia; `TOP` aún es cero.
4. Demo automática y sus inputs originales.
5. Finales A-Type/B-Type, cohetes y concierto.
6. Temporización exacta de título y menús.
7. Comparación de replays contra el núcleo C y referencia NES.
8. Validación en emuladores adicionales y hardware GBC real.

## Próximo bloque

Portar el RNG/one-reroll y los estados de lock/ARE; después añadir récords.
