# Port de Tetris NES a Game Boy Color — v0.2

Esta rama registra la segunda entrega incremental del port específico a Game Boy Color, sin audio. No es un emulador ni un convertidor universal. El usuario proporciona la ROM legal exacta de Tetris (USA), CRC32 `D16EA396`, y la herramienta genera localmente un cartucho CGB-only de 32 KiB.

## Implementado en v0.2

- Conversión de los 16 KiB CHR de NES al orden 2bpp intercalado de Game Boy.
- Decodificación de los streams PPU del marco y de la paleta desde el PRG legal.
- Uso del banco CHR 3 y de los tiles de bloque `0x7B`, `0x7C` y `0x7D` identificados por el port de PC.
- Campo lógico 10×20 con 18 filas visibles por el límite de 160×144 de GBC.
- Siete piezas, rotaciones, colisión, fijación, borrado y respawn.
- HUD compacto con `LINES`, `SCORE`, `NEXT` y `LEVEL`, usando la fuente de la ROM.
- Puntuación 40/100/300/1200 × (nivel+1), puntos de soft drop y gravedad de niveles 0–29.
- Paletas por nivel, subida de nivel cada diez líneas, DAS aproximado, pausa y game over.
- Ejecutables Windows GUI/CLI sin dependencias externas y plantilla LR35902 generada por Python.
- Pruebas de CPU para inicialización, input, caída, lock, respawn, línea, puntuación y level-up.

## Pendiente

- RNG de dos bytes y selección one-reroll idénticos.
- Estados y timing NES exactos: lock pending, row check, animación y ARE.
- Título, demo, menús A/B y selector de nivel/altura.
- Mode B, estadísticas, récords, entrada de nombre y finales.
- Validación en hardware Game Boy Color físico.

El código fuente completo de esta entrega se mantiene en el paquete de trabajo `NES2GBC_v0.2_Source.zip`; la ROM del usuario y el `.gbc` derivado no deben incorporarse al repositorio.
