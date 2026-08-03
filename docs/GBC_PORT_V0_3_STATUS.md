# Estado del port GBC v0.3

Esta rama registra el avance experimental del port específico **Tetris (USA) NES → Game Boy Color**, sin audio. El cartucho generado sigue requiriendo la ROM legal exacta del usuario y no almacena assets de Nintendo en el repositorio.

## Implementado

- Pantalla de título reconstruida desde PRG/CHR, con paneo horizontal 256→160 sin estirar.
- Pantalla original de selección A-Type/B-Type.
- Pantallas originales de nivel y altura.
- Selección A-Type nivel 0–9.
- Selección B-Type nivel 0–9 y altura 0–5.
- Cursores derivados de los sprites originales.
- B-Type funcional con 25 líneas y basura inicial por altura.
- Marco, fuente, bloques y paletas originales de la partida.
- Corrección del fallo de piezas que desaparecían al fijarse: un redibujado de 180 tiles no cabe en un VBlank real. Los redibujados completos ahora esperan VBlank, apagan el LCD, actualizan VRAM/OAM/paleta y lo reactivan.
- Prueba LR35902 con bloqueo de VRAM durante líneas visibles para detectar esta clase de error.

## Verificado

- Flujo título → tipo → nivel/altura → partida.
- Movimiento, soft drop, lock, respawn y persistencia visual del tablero.
- Borrado de línea, puntuación, cambio de nivel y paleta.
- B-Type: basura, contador 25→0 y estado de finalización.

## Pendiente

- RNG two-byte/one-reroll y basura B-Type exactos.
- STATISTICS.
- Estados y tiempos exactos de lock, row check, animación de línea y ARE.
- Récords, entrada de nombre, demo y finales.
- Validación en hardware GBC real.

El paquete compilado y el código fuente se mantienen fuera del repositorio hasta integrar el árbol completo en commits posteriores.