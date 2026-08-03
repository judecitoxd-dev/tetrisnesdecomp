# Port Tetris NES → Game Boy Color, prototipo v0.1

Esta rama reserva el trabajo del primer port específico a Game Boy Color. No es un emulador ni un convertidor universal de ROMs NES.

## Alcance alcanzado localmente

- conversor Windows x64 `nes2gbc.exe`;
- validación de la ROM legal exacta de Tetris (USA) mediante SHA-256 y CRC32;
- conversión de 16 KiB CHR NES a tiles 2bpp intercalados de Game Boy;
- plantilla `.gbc` ROM-only de 32 KiB, sin sonido;
- motor LR35902 básico jugable con tablero 10×20, siete piezas, rotación, gravedad, caída rápida, colisión, fijación, borrado de líneas y game over;
- pruebas sintéticas del conversor, encabezado/checksums y ejecución de 65 fotogramas del código LR35902.

## Estado

Es una prueba funcional inicial, no una conversión 1:1. Todavía faltan menús A/B, selector de nivel, puntuación, NEXT, DAS/timing exacto, paletas y pantallas de la ROM, modo B, demostraciones y finales. El banco CHR legal queda insertado en la salida, pero el tablero actual usa tiles provisionales originales.

El paquete fuente completo se preparó fuera del repositorio para no incluir ninguna ROM ni assets extraídos. Los siguientes commits deben incorporar el generador de plantilla, el conversor Go y las pruebas automatizadas.
