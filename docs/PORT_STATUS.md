# Estado de decompilación y ports

## Resumen de v0.10

La décima fase mantiene un único núcleo C99 para Windows, Linux y Android y
añade una ruta reproducible para comparar el modelo C contra capturas de un
emulador fotograma por fotograma.

`tetris_demo_verify` genera ahora el esquema de traza v2. La herramienta
`trace_compare.py` alinea ambas trazas por número de fotograma, normaliza valores
decimales, hexadecimales y binarios y localiza el primer campo divergente.

## Progreso estimado

| Área | Estado aproximado |
|---|---:|
| Port nativo jugable básico | 99% |
| Port Android jugable | 95% |
| Controles, tablero y puntuación | 99% |
| Modo A | 96% |
| Modo B | 92% |
| Integración y empaquetado | 98% |
| Carga legal de recursos desde ROM | 98% |
| Fidelidad de reglas y timings principales | 85% |
| Pantallas y animaciones originales/equivalentes | 94% |
| Efectos y música nativos no propietarios | 88% |
| Paquetes OGG del usuario en PC | 90% |
| Renderizado automático del APU original | 0% |
| Decompilación etiquetada/verificada del PRG 6502 | 35% |
| Correspondencia reproducible con la ROM | 8% |

Estas cifras son estimaciones por subsistema. La nueva cifra de correspondencia
indica que ya existe infraestructura de comparación; no afirma que el estado del
port coincida todavía con una ejecución 6502 real.

## Implementado

- Coordenadas, pivotes y orientaciones de las 19 configuraciones de tetrominós.
- Tabla NTSC de gravedad, puntuación, transiciones, RNG, borrado, entrada y DAS.
- Modos A y B, alturas, campo inicial, bonus, pausa, cortina de derrota y récords.
- Atlas de los cuatro bancos CHR leído desde la ROM legal.
- Título, selección de tipo, selección de nivel/altura y marco desde streams PPU.
- Pantallas de récords y entrada de nombre sobre la composición original.
- Cursores OAM de tipo, nivel/altura y nombre.
- Demo NTSC desde `PRG+0x5D00` y piezas desde `PRG+0x5F00`.
- Firma reproducible interna de la demo.
- Final B-Type normal y castillo de nivel 9/19.
- Cinco parches PPU acumulativos y seis composiciones de concierto por altura.
- Personajes del concierto desde metasprites OAM y tablas de animación.
- Final A-Type desde 30,000 puntos con cinco clases de cohete.
- Fondo A-Type normal y parche especial ≥120,000.
- Quince metasprites A-Type: cinco cuerpos y diez chorros.
- Orden final A-Type → nombre → tabla de récords.
- Renderer alternativo para ROMs compatibles cuyo CRC no coincide.
- Teclado, mando y controles multitáctiles sobre el mismo núcleo.
- Selector Android SAF, almacenamiento privado y proyecto ARM64/ARMv7.
- Opciones persistentes, última ROM, disposición táctil y dimensiones de ventana.
- Grabación y reproducción determinista con hash final.
- Audio sintetizado original y paquetes OGG opcionales del usuario.
- Desensamblador NMOS 6502 por bancos MMC1, símbolos y referencias cruzadas.
- Verificadores de estructuras PPU/OAM/demo/finales sin assets extraídos.
- Traza de demo v2 con entrada, tablero, RNG, DAS, fases y contadores.
- Comparador CSV con primer fotograma divergente e informe JSON.
- GitHub Actions configurado para reconstruir PR al recibir `synchronize`.

## Esquema de traza v2

Cada fila exportada por `tetris_demo_verify` contiene:

- `frame`, `state_hash` y `board_hash`;
- máscara `input`;
- `phase`, `active`, `next`, `x`, `y` y `rotation`;
- `score`, `lines`, `total_lines` y `level`;
- `spawn_count` y `rng_seed`;
- contadores de frame, caída, DAS, soft drop, fase y borrado;
- offsets consumidos de botones y piezas.

La herramienta no contiene una captura de RAM de Nintendo. El usuario genera
localmente tanto la traza del port como la referencia del emulador.

## Validación realizada para v0.10

- `trace_compare.py --self-test` pasa con trazas iguales y divergentes.
- El comparador acepta decimal, `0x`, `$`, `0b` y `%`.
- La modificación de `demo_verify.c` pasa análisis sintáctico C99 con
  `-Wall -Wextra -Wpedantic -Werror` usando las interfaces públicas.
- La traza conserva el hash acumulado anterior y añade hashes/columnas nuevas.
- Los workflows incluyen la autoprueba antes de compilar.
- Los resultados completos de Windows, Linux y Android deben confirmarse en el
  pull request de v0.10.

## Medición del desensamblado

El recorrido conservador desde vectores y símbolos conocidos identifica
aproximadamente 1,166 instrucciones / 2,441 bytes de código. El modo agresivo
identifica aproximadamente 3,839 instrucciones / 7,818 bytes, pero incluye
raíces heurísticas y no debe considerarse una medición exacta.

La traducción verificable todavía no produce objetos 6502 enlazables ni un PRG
idéntico.

## Diferencias conocidas

- Los números dinámicos no emulan escrituras PPU por ciclos.
- La demo usa entradas y piezas originales, pero las reglas se ejecutan en C.
- Aún no se ha añadido al repositorio una traza de RAM de emulador.
- La paridad A-Type se fija respecto al inicio del final.
- Falta reproducir toda la entrada/movimiento de la catedral B-Type.
- El controlador APU no está traducido.
- La generación B-Type no se ha comparado para todas las semillas.
- El RNG no reproduce automáticamente el estado de encendido de una NES concreta.
- No existe un enlazado que produzca el PRG original bit a bit.

## Próxima fase hacia exactitud

1. Crear un script de captura de RAM para un emulador con API de depuración.
2. Mapear variables 6502 a las columnas del esquema v2.
3. Ejecutar la demo y corregir la primera divergencia real.
4. Completar la máquina de movimiento de la catedral B-Type.
5. Traducir el controlador musical/APU y renderizar audio desde la ROM legal.
6. Comparar transiciones, cortina y finales contra PPU/OAM.
7. Etiquetar y traducir más rutinas de ambos bancos PRG.
8. Crear una construcción 6502 enlazable.
