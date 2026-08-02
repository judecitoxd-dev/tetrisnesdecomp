# Estado de decompilación y ports

## Resumen de v0.9

La novena fase mantiene un único núcleo C99 para Windows, Linux y Android y amplía el uso directo de la ROM legal a la demo NTSC, cursores OAM y finales A-Type/B-Type.

Los fondos se reconstruyen en memoria a partir de streams `bulkCopyToPpu`, atributos, paletas y bancos CHR. Los personajes, cursores, cohetes y chorros se generan desde `oamContentLookup`. Ningún recurso descomprimido se guarda en el repositorio o en los paquetes.

## Progreso estimado

| Área | Estado aproximado |
|---|---:|
| Port nativo jugable básico | 99% |
| Port Android jugable | 94% |
| Controles, tablero y puntuación | 99% |
| Modo A | 96% |
| Modo B | 92% |
| Integración y empaquetado | 97% |
| Carga legal de recursos desde ROM | 98% |
| Fidelidad de reglas y timings principales | 84% |
| Pantallas y animaciones originales/equivalentes | 94% |
| Efectos y música nativos no propietarios | 88% |
| Paquetes OGG del usuario en PC | 90% |
| Renderizado automático del APU original | 0% |
| Decompilación etiquetada/verificada del PRG 6502 | 34% |
| Correspondencia reproducible con la ROM | 0% |

Estas cifras son estimaciones por subsistema. Un port puede estar casi terminado como aplicación mientras la reconstrucción exacta del programa 6502 continúa siendo mucho más larga.

## Implementado

- Coordenadas, pivotes y orientaciones de las 19 configuraciones de tetrominós.
- Tabla NTSC de gravedad, puntuación, transiciones, RNG, borrado, entrada y DAS.
- Modos A y B, alturas, campo inicial, bonus, pausa, cortina de derrota y récords.
- Atlas de los cuatro bancos CHR leído desde la ROM legal.
- Título, selección de tipo, selección de nivel/altura y marco de partida desde streams PPU.
- Tablero alineado a la cuadrícula original de tiles de 8×8 escalada 2×.
- Tetrominós renderizados con los tiles originales `$7B`, `$7C` y `$7D`.
- Pantallas de récords y entrada de nombre sobre la composición original.
- Cursores OAM de tipo, nivel/altura y nombre.
- Demo NTSC desde `PRG+0x5D00` y piezas desde `PRG+0x5F00`.
- Firma reproducible de la demo: 4,785 fotogramas, 512 bytes de comandos y 40 piezas.
- Final B-Type normal y castillo de nivel 9/19.
- Cinco parches PPU acumulativos y seis composiciones de concierto por altura.
- Personajes del concierto desde metasprites OAM y tablas de animación.
- Final A-Type desde 30,000 puntos con cinco clases de cohete.
- Fondo A-Type normal y parche especial ≥120,000.
- Quince metasprites A-Type: cinco cuerpos y diez chorros.
- Orden final A-Type → nombre → tabla de récords.
- Renderer alternativo para ROMs compatibles cuyo CRC no coincide con el dump verificado.
- Teclado, mando y controles multitáctiles sobre el mismo núcleo.
- Selector Android SAF, almacenamiento privado y proyecto ARM64/ARMv7.
- Opciones persistentes, última ROM, disposición táctil y dimensiones de ventana.
- Grabación y reproducción determinista con hash final.
- Audio sintetizado original y paquetes OGG opcionales del usuario en escritorio.
- Desensamblador NMOS 6502 por bancos MMC1, símbolos, referencias cruzadas y espacio privado.
- Verificadores de estructuras PPU/OAM/demo/finales que no extraen assets.

## Validación local de v0.9

- `app.c` y `main.c` compilan como C99 con `-Wall -Wextra -Wpedantic -Werror`.
- Clang Static Analyzer no reporta diagnósticos en los módulos nuevos.
- 2/2 pruebas Release del núcleo pasan.
- La demo completa consume sus 512 bytes sin salir de rango.
- El harness A-Type carga dos fondos, cinco cohetes y quince metasprites sin faltantes.
- La ROM usada en las pruebas tiene CRC32 `D16EA396`.

## Estado real de GitHub Actions

Los últimos jobs Windows, Linux y Android aparecen en rojo. Terminan antes de crear el paso `Set up job`, devuelven `steps: null` y no producen logs ni artefactos. Un reintento manual reprodujo el mismo resultado.

Esto impide afirmar que v0.9 esté validada por CI o publicar un APK v0.9. El PR permanece abierto y `main` continúa en v0.8 hasta que los runners vuelvan a ejecutar pasos reales.

## Medición del desensamblado

El recorrido conservador desde vectores y símbolos conocidos identifica aproximadamente 1,166 instrucciones / 2,441 bytes de código. El modo agresivo identifica aproximadamente 3,839 instrucciones / 7,818 bytes, pero incluye raíces heurísticas y no debe considerarse una medición exacta de código confirmado.

La traducción verificable todavía no produce objetos 6502 enlazables ni un PRG idéntico.

## Diferencias conocidas

- Los números dinámicos se dibujan sobre fondos originales, pero sus actualizaciones no emulan el PPU por ciclos.
- La demo usa entradas y piezas originales, pero las reglas siguen ejecutándose en el modelo C.
- La paridad de animación A-Type se fija respecto al inicio del final, no al contador global de una NES concreta.
- Falta reproducir toda la entrada/movimiento de la catedral B-Type.
- El controlador APU no está traducido; Android usa el sintetizador y PC puede usar OGG creados localmente.
- La generación B-Type no se ha comparado para todas las semillas y fotogramas.
- El RNG no reproduce automáticamente el estado exacto de encendido de una consola concreta.
- No existe un enlazado que produzca el PRG original bit a bit.

## Próxima fase hacia exactitud

1. Capturar estados de RAM de la demo en un emulador y compararlos fotograma por fotograma.
2. Completar la máquina de movimiento de la catedral B-Type.
3. Traducir el controlador musical/APU y renderizar audio desde la ROM legal.
4. Comparar transiciones, cortina y finales contra capturas de PPU/OAM.
5. Etiquetar y traducir más rutinas de ambos bancos PRG.
6. Crear una construcción 6502 enlazable.
7. Perseguir correspondencia binaria solo después de tener segmentos y datos completos.
