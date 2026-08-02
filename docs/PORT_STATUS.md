# Estado de decompilación y ports

## Resumen de v0.7

La séptima fase mantiene un único núcleo C99 para Windows, Linux y Android y amplía el uso directo de la ROM legal. Además del título, fuente, bloques y paletas, ahora se interpretan los streams comprimidos usados por `bulkCopyToPpu` para reconstruir la selección de tipo, la selección de nivel y el marco principal de partida.

El port crea nametables virtuales en memoria, aplica atributos y paletas y dibuja los tiles del banco CHR correspondiente. Ningún recurso descomprimido se guarda en el repositorio o en los paquetes.

## Progreso estimado

| Área | Estado aproximado |
|---|---:|
| Port nativo jugable básico | 99% |
| Port Android jugable | 92% |
| Controles, tablero y puntuación | 99% |
| Modo A | 94% |
| Modo B | 88% |
| Integración y empaquetado | 97% |
| Carga legal de recursos desde ROM | 96% |
| Fidelidad de reglas y timings principales | 81% |
| Pantallas y animaciones originales/equivalentes | 86% |
| Efectos y música nativos no propietarios | 88% |
| Paquetes OGG del usuario en PC | 90% |
| Renderizado automático del APU original | 0% |
| Decompilación etiquetada/verificada del PRG 6502 | 31% |
| Correspondencia reproducible con la ROM | 0% |

Estas cifras son estimaciones por subsistema. Un port puede estar casi terminado como aplicación mientras la reconstrucción exacta del programa 6502 continúa siendo mucho más larga.

## Implementado

- Coordenadas, pivotes y orientaciones de las 19 configuraciones de tetrominós.
- Tabla NTSC de gravedad, puntuación, transiciones, RNG aproximado al cartucho, borrado, entrada y DAS.
- Modos A y B, alturas, campo inicial, bonus, pausa, cortina de derrota y récords.
- Atlas completo de los cuatro bancos CHR leído desde la ROM legal.
- Título reconstruido desde su flujo PPU original.
- Selección de tipo reconstruida desde el PRG y CHR originales.
- Selección de nivel/altura reconstruida desde el PRG, incluido el parche de A-Type.
- Marco de partida reconstruido desde nametable, atributos, paleta y banco CHR de juego.
- Tablero alineado a la cuadrícula original de tiles de 8×8 escalada 2×.
- Tetrominós renderizados con los tiles originales `$7B`, `$7C` y `$7D`.
- Valores dinámicos superpuestos: puntuación, líneas, nivel, pieza siguiente, estadísticas y cursores.
- Renderer alternativo para ROMs compatibles cuyo CRC no coincide con el dump verificado.
- Teclado, mando y controles multitáctiles sobre el mismo núcleo.
- Selector Android SAF, almacenamiento privado y APK ARM64/ARMv7.
- Opciones persistentes, última ROM, disposición táctil y dimensiones de ventana.
- Grabación y reproducción determinista con hash final.
- Audio sintetizado original y paquetes OGG opcionales del usuario en escritorio.
- CMake/CPack para Windows/Linux y Gradle/NDK para Android.
- Desensamblador NMOS 6502 por bancos MMC1, símbolos, referencias cruzadas y espacio privado.

## Medición del desensamblado

El recorrido conservador desde vectores y símbolos conocidos identifica aproximadamente 1,166 instrucciones / 2,441 bytes de código. El modo agresivo identifica aproximadamente 3,839 instrucciones / 7,818 bytes, pero incluye raíces heurísticas y no debe considerarse una medición exacta de código confirmado.

La traducción verificable todavía no produce objetos 6502 enlazables ni un PRG idéntico.

## Diferencias conocidas

- Los cursores de menús son equivalentes SDL; aún no se reproducen desde OAM.
- Los números dinámicos se dibujan sobre el fondo original, pero su actualización no emula el PPU por ciclos.
- Récords y entrada de nombre no usan todavía toda la composición y las tablas originales.
- La demostración utiliza el controlador determinista del port, no las entradas exactas almacenadas en la ROM.
- Los finales, cohetes, músicos y sprites OAM siguen siendo equivalentes parciales.
- El controlador APU no está traducido; Android usa el sintetizador y PC puede usar OGG creados localmente.
- La generación B-Type no se ha comparado para todas las semillas y fotogramas.
- El RNG no reproduce automáticamente el estado exacto de encendido de una consola concreta.
- No existe un enlazado que produzca el PRG original bit a bit.

## Validación prevista para v0.7

- Compilación y pruebas Release en Windows y Linux.
- APK para `arm64-v8a` y `armeabi-v7a`.
- Comprobación de que los paquetes no incluyan `.nes`, `.ttr` ni `.ogg`.
- Arranque headless del artefacto Linux con el dump CRC32 `D16EA396`.
- Inspección del APK y del ejecutable Windows.
- Comparación visual de los fondos decodificados con las nametables del desensamblado público.

## Próxima fase hacia exactitud

1. Reconstruir récords y entrada de nombre desde los streams PPU y tablas originales.
2. Interpretar las tablas OAM y metasprites de cursores y finales.
3. Reproducir la demo usando la tabla de entradas del cartucho.
4. Comparar B-Type y transiciones fotograma por fotograma.
5. Traducir el controlador musical/APU y renderizar audio desde la ROM legal.
6. Etiquetar y traducir más rutinas de ambos bancos PRG.
7. Crear una construcción 6502 enlazable.
8. Perseguir correspondencia binaria solo después de tener segmentos y datos completos.
