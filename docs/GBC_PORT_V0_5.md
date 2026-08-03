# Port GBC v0.5

Estado de la conversión específica de Tetris (USA) NES a Game Boy Color, sin sonido.

## Cambios de esta revisión

- Pausa centrada sobre el tablero mediante OAM; no tapa TOP, SCORE, NEXT ni LEVEL.
- Arriba y A rotan en sentido horario; B rota en sentido antihorario.
- Prueba explícita que alinea el suelo lógico con la fila 20 visible.
- Cabecera de juego desplazada hacia abajo para dejar margen superior.
- Marcos compactos nuevos con margen negro interno para separar visualmente las cajas.
- Cursores de GAME TYPE y LEVEL/HEIGHT recolocados dentro de sus paneles.
- Espaciado de nivel reducido de 32 a 24 píxeles para evitar que el selector salga del marco.
- Tablero completo 10x20, A-Type, B-Type, estadísticas, score, next y level conservados.

## Verificación ejecutada

- Flujo título -> tipo -> nivel/altura -> partida.
- Lock y persistencia visual bajo restricciones reales de VRAM.
- Fila 20 ocupada correctamente por una pieza O.
- Rotación con Arriba/A/B.
- Pausa centrada sin modificar los tiles de SCORE.
- Generación con la ROM verificada CRC32 D16EA396.

## Pendiente

RNG one-reroll exacto, lock/ARE y animación de líneas exactos, récords persistentes, demo, finales y sonido.
