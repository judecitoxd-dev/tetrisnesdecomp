# Port Game Boy Color v0.6

Avance incremental del conversor específico de Tetris (USA) NES a Game Boy Color, sin sonido.

## Cambios de esta revisión

- El panel de estadísticas y el tablero comparten un borde para liberar una columna al HUD derecho.
- Los seis dígitos de TOP y SCORE, y los dos de LV, permanecen dentro de sus marcos.
- El borde inferior de juego está alineado con la fila lógica 20; las piezas fijadas tocan visualmente el suelo.
- Los bloques compactos 4x4 conservan las sombras del tile NES, pero ocupan toda la celda para leerse mejor.
- La pieza activa se desplazó junto con el tablero para mantener coincidencia exacta entre render y colisión.
- La torre del título se recolocó en la esquina inferior derecha y el copyright se separó a la izquierda.
- Se conservan pausa centrada, rotación con Arriba/A y giro contrario con B.

## Verificación

La plantilla y el cartucho generado pasan pruebas LR35902 para menús, A-Type, B-Type, colisión con suelo, fila 20, lock, borrado de líneas, score, nivel, pausa y restricciones de acceso a VRAM.

La ROM ni sus recursos derivados se incluyen en el repositorio. El paquete local se genera desde la copia legal del usuario.
