# Estado de decompilación y port

## Resumen de v0.2

La ROM comprobada está identificada como NES 2.0, mapper MMC1, 32 KiB de PRG y 16 KiB de CHR. El
port carga y valida la ROM legal del usuario, localiza PRG/CHR, calcula CRC32 y decodifica tiles CHR
en tiempo de ejecución. La lógica jugable vive en una biblioteca C independiente de SDL; la capa
de escritorio usa SDL2 para vídeo, entrada, mandos y audio sintetizado.

En esta fase se sustituyeron varias aproximaciones de v0.1 por traducciones contrastadas del código
6502: orientaciones, gravedad, randomizador, puntuación, progresión de nivel y borrado de líneas.

## Progreso por área

| Área | Estado aproximado |
|---|---:|
| Port nativo jugable básico | 88% |
| Controles, tablero y puntuación | 94% |
| Carga legal de recursos desde ROM | 78% |
| Fidelidad de reglas y timings principales | 72% |
| Efectos de sonido nativos sintetizados | 65% |
| Audio original del cartucho | 0% |
| Pantallas y animaciones originales | 35% |
| Decompilación etiquetada del PRG 6502 | 20% |
| Correspondencia reproducible con la ROM | 0% |

Son estimaciones de ingeniería por subsistema, no un porcentaje automático del binario. Un port
jugable puede estar avanzado aunque la decompilación reproducible siga en una fase temprana.

## Completado

- Coordenadas y pivotes de las 19 orientaciones de tetrominós.
- Orientación inicial de cada pieza y rotación sin wall-kicks.
- Tabla NTSC de gravedad de 30 entradas.
- LFSR de dos bytes y selección con una repetición.
- Puntuación 40/100/300/1200 multiplicada por nivel más uno.
- Comparación de líneas en BCD, incluidas las transiciones especiales de niveles iniciales 10–19.
- Animación de borrado centro hacia afuera durante 20 fotogramas.
- Retardo de entrada dependiente de altura, DAS, caída suave y pausa.
- Cortina de fin de partida, título, selector de nivel y estadísticas.
- Entrada persistente de teclado y mando para no perder pulsaciones cortas.
- Efectos sintetizados sin incorporar datos de audio del cartucho.
- Pruebas de aparición, gravedad, niveles, borrado, puntuación, entrada y RNG determinista.
- Mapa inicial de vectores y tablas pequeñas verificables con la ROM legal.

## Diferencias conocidas

- El retardo de entrada reproduce la tabla temporal conocida, pero aún falta validar cada transición
  por fotograma contra una ejecución instrumentada del cartucho.
- El DAS y la caída suave están mucho más cerca del original, pero todavía pueden diferir en casos
  límite donde coinciden bloqueo, gravedad, rotación y lectura del mando.
- La caída instantánea es una extensión de PC y está separada de las reglas normales.
- Los efectos son ondas sintetizadas; no son las composiciones ni el driver APU originales.
- El título y el selector son recreaciones funcionales, no copias de las pantallas originales.
- Las paletas siguen convertidas a RGB por el port; la tabla del PRG está localizada, pero no se usa.
- No están implementados modo B, altura inicial, récords, finales, cohetes ni demostraciones.
- El juego todavía no genera un binario equivalente al PRG original.

## Próxima fase técnica

1. Instrumentar un emulador de referencia y registrar estado por fotograma para secuencias fijas.
2. Comparar DAS, caída suave, bloqueo, entrada y pausa en casos límite.
3. Conectar las paletas por nivel encontradas en PRG al renderizado RGB.
4. Traducir el flujo completo de menús y añadir modo B con selección de altura.
5. Etiquetar las rutinas del controlador APU y recrear música/efectos sin redistribuir la ROM.
6. Añadir récords, animaciones finales y pruebas deterministas por hashes de estado.
7. Separar gradualmente módulos C para que cada rutina traducida conserve referencias al offset 6502.
