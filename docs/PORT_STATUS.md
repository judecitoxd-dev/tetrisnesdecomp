# Estado de decompilación y port

## Resumen de v0.5

La quinta fase convierte la base v0.4 en una candidata seria a producto de escritorio: añade
configuración persistente, selector de ROM, paquetes portables y un sistema de repeticiones que
registra las entradas por fotograma y verifica el estado final. También inicia una línea de trabajo
más rigurosa para la decompilación con un desensamblador recursivo 6502, símbolos, referencias
cruzadas y espacios de trabajo privados generados desde la ROM del usuario.

La ROM comprobada sigue siendo NES 2.0, mapper MMC1, con 32 KiB de PRG y 16 KiB de CHR. El port
carga la copia legal del usuario y no incorpora bytes propietarios en el repositorio ni en los
paquetes generados.

## Progreso estimado

| Área | Estado aproximado |
|---|---:|
| Port nativo jugable básico | 99% |
| Controles, tablero y puntuación | 99% |
| Modo A | 93% |
| Modo B | 87% |
| Integración de escritorio y empaquetado | 94% |
| Carga legal de recursos desde ROM | 92% |
| Fidelidad de reglas y timings principales | 80% |
| Efectos y música nativos no propietarios | 88% |
| Reproducción de paquetes OGG del usuario | 90% |
| Renderizado automático del APU original | 0% |
| Pantallas y animaciones originales/equivalentes | 78% |
| Decompilación etiquetada/verificada del PRG 6502 | 28% |
| Correspondencia reproducible con la ROM | 0% |

Son estimaciones de ingeniería por subsistema, no un porcentaje automático de bytes traducidos. El
port puede estar casi completo como aplicación mientras la reconstrucción exacta del programa 6502
continúa siendo una fase mucho más larga.

## Implementado

- Coordenadas, pivotes y orientaciones de las 19 configuraciones de tetrominós.
- Tabla NTSC de gravedad, puntuación, transiciones especiales y selector de piezas basado en LFSR.
- Animación de borrado, retardo de entrada, DAS, caída suave, pausa y cortina de derrota.
- Modos A y B con objetivo, alturas, campo inicial y bonus de finalización.
- Atlas completo de los cuatro bancos CHR de 4 KiB leído directamente de la ROM legal.
- Pantalla de título reconstruida desde su flujo de copia PPU, nametable, atributos y paleta de la ROM.
- Tetrominós renderizados con los tiles originales `$7B`, `$7C` y `$7D` del banco CHR de juego.
- Paletas de nivel leídas directamente de la ROM legal.
- Reproducción opcional de tres músicas y ocho efectos OGG creados localmente por el usuario.
- Conversor FFmpeg, búsqueda automática del paquete y retorno seguro al sintetizador.
- Menús nativos, récords con nombres editables, demostración y final equivalente de B-Type.
- Teclado y mando, cola de pulsaciones y conexión de controladores en caliente.
- Opciones persistentes, última ROM, última configuración y dimensiones de ventana.
- Selector de ROM, arrastrar y soltar y argumentos `--rom` / `--replay`.
- Grabación automática y reproducción determinista con hash final de estado.
- Verificador de repeticiones por consola.
- Compilación, instalación y paquetes CPack para Windows y Linux.
- Pruebas siempre activas también en Release, sanitizadores y análisis estático.
- Desensamblador recursivo oficial NMOS 6502 con bancos MMC1, símbolos y referencias cruzadas.
- Generador de espacio de trabajo privado con hashes y listados conservador/agresivo.

## Medición del desensamblado

Con la ROM comprobada, el recorrido conservador desde vectores y símbolos conocidos identifica
aproximadamente **1,166 instrucciones / 2,441 bytes de código (7.45% del PRG)**. El modo agresivo
encuentra aproximadamente **3,839 instrucciones / 7,818 bytes (23.86%)**, pero usa raíces
heurísticas y por tanto **no debe interpretarse como código verificado ni progreso final**.

El porcentaje de “decompilación etiquetada/verificada” de la tabla incluye además rutinas ya
contrastadas y traducidas al núcleo C. No equivale a una reconstrucción enlazable ni idéntica.

## Diferencias conocidas

- La generación y temporización de B-Type aún no se comparan por fotograma para todas las semillas.
- La caída instantánea, niveles B 10–19 directos y las pistas chiptune son mejoras de PC.
- El título y los bloques ya reproducen recursos PPU originales; los demás menús y sprites de final todavía no.
- El final y la demostración son equivalentes nativos, no traducciones exactas de rutinas o entradas.
- El controlador APU del cartucho no está traducido; el paquete OGG todavía requiere capturas o un render externo.
- El RNG no comparte automáticamente el contador exacto de una consola desde el encendido.
- Las repeticiones v0.5 son locales al modelo de estado actual; futuras versiones incompatibles
  deben incrementar la versión del formato.
- No existe todavía un enlazado que produzca un PRG idéntico al original.

## Validación de v0.5

- Núcleo y herramientas compilados como C99 con advertencias convertidas en errores.
- Pruebas unitarias ejecutadas realmente en Release mediante comprobaciones independientes de
  `assert` y `NDEBUG`.
- Pruebas con AddressSanitizer y UndefinedBehaviorSanitizer aprobadas.
- Aplicación completa configurada, compilada, instalada y empaquetada mediante CMake/CPack.
- Arranque validado con la ROM CRC32 `D16EA396` y controladores de vídeo/audio virtuales.
- Repetición real de 1,800 fotogramas guardada, cargada y verificada con hash idéntico.
- Archivos dañados o con datos sobrantes son rechazados por el cargador de repeticiones.
- Autoprueba del desensamblador 6502 aprobada y espacio de trabajo privado generado correctamente.
- Análisis estático de Clang sin diagnósticos en los módulos revisados.

En este entorno no estaban instalados los encabezados oficiales de SDL2; la integración completa se
validó con una interfaz ABI de prueba y la biblioteca de ejecución. GitHub Actions compila contra
SDL2 real en Windows y Linux.

## Próxima fase

1. Ejecutar pruebas prolongadas con los artefactos reales de Windows y Linux.
2. Añadir hashes intermedios por fotograma y una suite de repeticiones de regresión sin ROM.
3. Comparar B-Type, menús y transiciones contra un emulador de referencia.
4. Separar y etiquetar más rutinas de los dos bancos PRG del MMC1.
5. Decodificar los nametables restantes y las tablas OAM de finales directamente desde la ROM.
6. Identificar el controlador musical/APU y conectarlo al formato OGG ya soportado sin distribuir datos extraídos.
7. Crear una construcción enlazable del programa 6502 antes de perseguir correspondencia binaria.
