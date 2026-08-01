# Estado de decompilación y port

## Resumen de v0.4

La ROM comprobada es NES 2.0, mapper MMC1, con 32 KiB de PRG y 16 KiB de CHR. El port carga la copia
legal del usuario, valida la cabecera, calcula CRC32, localiza PRG/CHR y decodifica recursos durante
la ejecución. La lógica jugable está separada de SDL2 para poder probarla sin abrir una ventana.

La cuarta fase completa varias funciones visibles de una versión de escritorio: nombres editables
para récords, demostración automática que juega el núcleo real y una celebración animada al superar
B-Type. El controlador de demo no usa una grabación: busca una colocación para cada pieza y penaliza
huecos, altura y desniveles.

## Progreso estimado

| Área | Estado aproximado |
|---|---:|
| Port nativo jugable básico | 97% |
| Controles, tablero y puntuación | 98% |
| Modo A | 91% |
| Modo B | 84% |
| Carga legal de recursos desde ROM | 84% |
| Fidelidad de reglas y timings principales | 78% |
| Efectos y música nativos no propietarios | 82% |
| Audio original del cartucho | 0% |
| Pantallas y animaciones equivalentes | 68% |
| Decompilación etiquetada del PRG 6502 | 24% |
| Correspondencia reproducible con la ROM | 0% |

Son estimaciones de ingeniería por subsistema. No representan cuántos bytes de la ROM se han
traducido automáticamente.

## Implementado

- Coordenadas, pivotes y orientaciones de las 19 configuraciones de tetrominós.
- Orientación inicial por pieza y rotación sin wall-kicks.
- Tabla NTSC de gravedad de 30 entradas.
- LFSR de dos bytes y selector de pieza con una repetición.
- Puntuación 40/100/300/1200 multiplicada por nivel más uno.
- Transiciones especiales de nivel inicial en modo A.
- Animación de borrado centro hacia afuera durante 20 fotogramas.
- Retardo de entrada dependiente de la altura, DAS, caída suave y pausa.
- Cortina de derrota, título, selección de tipo y configuración de nivel/altura.
- Modo B con 25 líneas restantes, nivel fijo, alturas 0–5 y bonus de finalización.
- Generación determinista de basura de modo B con al menos un hueco por fila.
- Estadísticas por pieza y siguiente pieza conmutable.
- Teclado y mando con cola de pulsaciones cortas.
- Tres récords persistentes por modo, incluyendo empates e introducción editable de nombre.
- Demostración automática con juego real y controlador de colocación determinista.
- Final animado básico para completar B-Type.
- Paletas de nivel obtenidas de la ROM legal.
- Efectos sintetizados y tres composiciones chiptune nuevas.
- Pruebas de aparición, gravedad, niveles, borrado, puntuación, entrada, RNG, modo B y récords.
- Mapa inicial de vectores y tablas verificables.

## Diferencias conocidas

- La generación de modo B sigue las tablas y flujo identificados, pero todavía no se ha comparado
  fotograma por fotograma contra cada combinación de nivel, altura y semilla del cartucho.
- La caída instantánea, niveles B 10–19 directos y las pistas chiptune son mejoras de PC.
- Los colores de bloque usan los índices de las paletas de nivel, pero el sombreado y los sprites no
  reproducen todavía toda la composición PPU original.
- El final y la demostración son equivalentes nativos, no traducciones exactas de las rutinas,
  entradas grabadas, sprites o tiempos del cartucho.
- La música del cartucho y su controlador APU no están traducidos.
- El RNG no comparte el contador exacto de fotogramas de una consola desde el encendido, por lo que
  una partida nueva no genera la misma secuencia sin controlar la semilla.
- No existe todavía un proceso de recompilación que produzca un PRG idéntico al original.

## Validación de v0.4

- Núcleo compilado como C99 con advertencias estrictas.
- Pruebas unitarias: 100% aprobadas.
- Interfaz SDL2 validada sintácticamente como C99 con advertencias convertidas en errores.
- Análisis estático de Clang de ocho módulos sin diagnósticos.
- Prueba de demostración ejecutada durante miles de fotogramas y al menos veinte piezas.
- Herramienta de ROM confirmó vectores y cinco tablas pequeñas.

## Próxima fase

1. Comparar el modo B y los tiempos de menú fotograma por fotograma contra un emulador de referencia.
2. Traducir las pantallas y rutinas de final del PRG en lugar de usar equivalentes nativos.
3. Identificar las estructuras del controlador musical y del APU sin distribuir datos extraídos.
4. Añadir reproducción determinista de entradas y hashes de estado por fotograma.
5. Construir un desensamblado enlazable con símbolos y bancos MMC1 separados.
6. Crear pruebas de correspondencia entre funciones C y rutinas 6502 identificadas.
