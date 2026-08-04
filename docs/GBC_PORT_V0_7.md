# GBC port v0.7

Estado registrado para la entrega externa del port compacto a Game Boy Color.

## Implementado

- Cartucho CGB de 64 KiB con MBC1, RAM y batería.
- Interfaz 160×144 y tablero completo 10×20.
- A-Type y B-Type con nivel y altura.
- Récords top-3 separados para A/B y persistentes en SRAM.
- Tabla de nombres y puntuaciones después de seleccionar el modo.
- Entrada de tres iniciales después de Game Over cuando la puntuación clasifica.
- TOP de la partida conectado al récord guardado.
- Animación de nuevo récord con cohete y llama extraídos de la ROM legal.
- Restauración de las tres variantes de textura/color usadas por tablero, pieza activa, NEXT y estadísticas.
- Pruebas LR35902 de juego, VRAM, SRAM, récords, nombre y cohete.

## Pendiente

- Sonido y música.
- RNG/one-reroll exactos.
- Lock, row-check, line clear y ARE exactos.
- Demo y finales/concierto completos.
- Prueba en hardware GBC físico.

El código fuente completo y los ejecutables de esta entrega se distribuyen en el paquete v0.7 generado fuera del repositorio. No se añade la ROM del usuario al repositorio.
