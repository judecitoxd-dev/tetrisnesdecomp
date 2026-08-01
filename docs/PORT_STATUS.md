# Estado de decompilación y port

## Qué se completó en esta primera entrega

La ROM fue identificada como NES 2.0, mapper MMC1, 32 KiB de PRG y 16 KiB de CHR. Se añadió un
cargador que valida la cabecera, calcula CRC32, localiza PRG/CHR y decodifica los tiles gráficos en
tiempo de ejecución. La lógica jugable se reescribió en C independiente de SDL, y la capa de PC se
implementó con SDL2.

## Progreso por área

| Área | Estado aproximado |
|---|---:|
| Port nativo jugable básico | 75% |
| Controles, tablero y puntuación | 85% |
| Carga legal de recursos desde ROM | 70% |
| Fidelidad de timings y reglas NES | 45% |
| Audio original | 0% |
| Pantallas y animaciones originales | 10% |
| Decompilación exacta del PRG 6502 | 5% |
| Correspondencia reproducible con la ROM | 0% |

Estos porcentajes son estimaciones de ingeniería, no porcentajes automáticos del binario.

## Diferencias conocidas

- La progresión de nivel usa diez líneas por nivel; todavía falta reproducir las reglas especiales
  de nivel inicial del original.
- El randomizador imita la idea de una repetición, pero no comparte el estado RNG exacto del código
  6502.
- No se han implementado las animaciones de borrado, cohetes, finales ni tablas de récords.
- No hay música ni efectos de sonido.
- La caída instantánea es una mejora opcional de PC y no existía en el cartucho.
- Las paletas se recrean en RGB; aún no se leen las tablas exactas desde PRG.

## Siguiente fase recomendada

1. Crear un mapa de bancos MMC1 y un desensamblado etiquetado de los 32 KiB de PRG.
2. Identificar tablas de piezas, gravedad, puntuación, paletas, música y estados de pantalla.
3. Sustituir gradualmente las aproximaciones de `src/game.c` por traducciones comprobadas del 6502.
4. Comparar entradas y estados por frame contra un emulador de referencia.
5. Implementar APU o convertir el driver musical a una capa de audio nativa sin distribuir datos.
6. Añadir reproducción determinista y pruebas de regresión por hashes de estado.
