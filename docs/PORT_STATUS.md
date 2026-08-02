# Estado de decompilación y ports

## Resumen de v0.6

La sexta fase mantiene un único núcleo C99 para Windows, Linux y Android. Android usa el frontend SDL2 existente, selector SAF, almacenamiento privado para la ROM legal, pantalla horizontal inmersiva y una capa táctil editable. La posición, escala y opacidad de los controles se guardan y la capa se oculta automáticamente al detectar un gamepad.

El título, la fuente, los bloques y las paletas continúan leyéndose desde la ROM del usuario. La decompilación exacta 6502 sigue en progreso: todavía faltan traducciones exactas de menús, OAM, PPU, APU y finales, y aún no existe una recompilación que produzca un PRG idéntico.

## Progreso estimado

| Área | Estado aproximado |
|---|---:|
| Port nativo jugable básico | 99% |
| Port Android jugable | 88% |
| Controles, tablero y puntuación | 99% |
| Modo A | 93% |
| Modo B | 87% |
| Integración de escritorio y empaquetado | 95% |
| Carga legal de recursos desde ROM | 92% |
| Fidelidad de reglas y timings principales | 80% |
| Efectos y música nativos no propietarios | 88% |
| Reproducción de paquetes OGG del usuario en PC | 90% |
| Renderizado automático del APU original | 0% |
| Pantallas y animaciones originales/equivalentes | 78% |
| Decompilación etiquetada/verificada del PRG 6502 | 28% |
| Correspondencia reproducible con la ROM | 0% |

Son estimaciones de ingeniería por subsistema, no un porcentaje automático de bytes traducidos. El port puede estar casi completo como aplicación mientras la reconstrucción exacta del programa 6502 continúa siendo una fase mucho más larga.

## Implementado

- Coordenadas, pivotes y orientaciones de las 19 configuraciones de tetrominós.
- Tabla NTSC de gravedad, puntuación, transiciones especiales y selector de piezas basado en LFSR.
- Animación de borrado, retardo de entrada, DAS, caída suave, pausa y cortina de derrota.
- Modos A y B con objetivo, alturas, campo inicial y bonus de finalización.
- Atlas completo de los cuatro bancos CHR de 4 KiB leído directamente de la ROM legal.
- Pantalla de título reconstruida desde su flujo de copia PPU, nametable, atributos y paleta de la ROM.
- Tetrominós renderizados con los tiles originales `$7B`, `$7C` y `$7D` del banco CHR de juego.
- Paletas de nivel leídas directamente de la ROM legal.
- Menús nativos, récords con nombres editables, demostración y final equivalente de B-Type.
- Teclado, mando y capa táctil que generan las mismas entradas para el núcleo compartido.
- Selector Android SAF, validación iNES/MMC1 y almacenamiento privado de la ROM.
- Controles Android movibles con tamaño, opacidad y posición persistentes.
- Ocultación automática de los controles táctiles cuando se conecta un gamepad.
- Opciones persistentes, última ROM, última configuración y dimensiones de ventana.
- Grabación automática y reproducción determinista con hash final de estado.
- Verificador de repeticiones por consola.
- Reproducción opcional de tres músicas y ocho efectos OGG creados localmente por el usuario en escritorio.
- Conversor FFmpeg, búsqueda automática del paquete y retorno seguro al sintetizador.
- Compilación, instalación y paquetes CPack para Windows y Linux.
- APK Android para ARM64 y ARMv7 creado mediante Gradle, NDK y SDL2 oficial.
- Pruebas siempre activas también en Release, sanitizadores y análisis estático.
- Desensamblador recursivo oficial NMOS 6502 con bancos MMC1, símbolos y referencias cruzadas.
- Generador de espacio de trabajo privado con hashes y listados conservador/agresivo.

## Medición del desensamblado

Con la ROM comprobada, el recorrido conservador desde vectores y símbolos conocidos identifica aproximadamente **1,166 instrucciones / 2,441 bytes de código (7.45% del PRG)**. El modo agresivo encuentra aproximadamente **3,839 instrucciones / 7,818 bytes (23.86%)**, pero usa raíces heurísticas y por tanto **no debe interpretarse como código verificado ni progreso final**.

El porcentaje de “decompilación etiquetada/verificada” incluye además rutinas ya contrastadas y traducidas al núcleo C. No equivale a una reconstrucción enlazable ni idéntica.

## Diferencias conocidas

- La generación y temporización de B-Type aún no se comparan por fotograma para todas las semillas.
- La caída instantánea, niveles B 10–19 directos y las pistas chiptune son mejoras opcionales del port.
- El título y los bloques ya reproducen recursos PPU originales; los demás menús y sprites de final todavía no.
- El final y la demostración son equivalentes nativos, no traducciones exactas de rutinas o entradas.
- El controlador APU del cartucho no está traducido; el paquete OGG todavía requiere capturas o un render externo.
- Los paquetes OGG aún no se seleccionan desde Android; Android usa el sintetizador incorporado.
- El RNG no comparte automáticamente el contador exacto de una consola desde el encendido.
- Las repeticiones son locales al modelo de estado actual; futuras versiones incompatibles deben incrementar la versión del formato.
- No existe todavía un enlazado que produzca un PRG idéntico al original.

## Validación de v0.6

- Núcleo y herramientas compilados como C99 con advertencias estrictas.
- Pruebas unitarias ejecutadas realmente en Release mediante comprobaciones independientes de `assert` y `NDEBUG`.
- Windows y Linux configurados, compilados, probados, instalados y empaquetados mediante CMake/CPack.
- Android configurado con Java 17, SDK 34, NDK 26.3, CMake 3.22.1 y SDL2 2.30.11.
- APK compilado para `arm64-v8a` y `armeabi-v7a`.
- APK inspeccionado para confirmar `libSDL2.so`, `libmain.so`, manifiesto y clases Java en ambas arquitecturas.
- Comprobación automática y manual de que el APK no contiene `.nes`, `.ttr` ni `.ogg`.
- APK firmado como build de depuración e integridad ZIP verificada.
- Autoprueba del desensamblador 6502 aprobada.
- La ROM CRC32 `D16EA396` se usa solo durante pruebas locales y nunca se sube al repositorio ni a los paquetes.

No se ha ejecutado todavía el APK en una colección amplia de teléfonos físicos. La compilación, firma, contenido y arquitecturas están verificadas, pero quedan pruebas de uso prolongado, suspensión/reanudación, diferentes relaciones de aspecto y mandos Bluetooth.

## Próxima fase hacia exactitud

1. Ejecutar pruebas prolongadas en dispositivos Android físicos y artefactos reales de PC.
2. Añadir hashes intermedios por fotograma y una suite de repeticiones de regresión sin ROM.
3. Comparar B-Type, menús y transiciones contra una consola/emulador de referencia.
4. Separar y etiquetar más rutinas de los dos bancos PRG del MMC1.
5. Decodificar los nametables restantes y las tablas OAM de finales directamente desde la ROM.
6. Identificar y traducir el controlador musical/APU.
7. Crear una construcción enlazable del programa 6502.
8. Solo después perseguir correspondencia binaria con el PRG original.
