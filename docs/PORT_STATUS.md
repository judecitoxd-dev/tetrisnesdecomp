# Estado de decompilación y ports

## Resumen de v0.14

La decimocuarta fase cierra dos pendientes de verificación: la entrada móvil de
la catedral B-Type y la ejecución sistemática de todo el contenido APU ya
identificado.

La catedral usa ahora una máquina incremental que refleja literalmente los
contadores `ending`, `ending_customVars`, `ending_currentSprite` y `$00CD` del
6502. La herramienta `apu_matrix.py` genera o compara diez pistas y ocho
escenarios de efectos sin incluir ROM, música ni trazas propietarias.

## Progreso estimado

| Área | Estado aproximado |
|---|---:|
| Port nativo jugable básico | 99% |
| Port Android jugable | 97% |
| Controles, tablero y puntuación | 99% |
| Modo A | 96% |
| Modo B | 96% |
| Integración y empaquetado | 99% |
| Carga legal de recursos desde ROM | 99% |
| Fidelidad de reglas y timings principales | 86% |
| Pantallas y animaciones originales/equivalentes | 97% |
| Audio original interactivo | 95% |
| Renderizado automático del APU original | 92% |
| Decompilación etiquetada/verificada del PRG 6502 | 41% |
| Correspondencia reproducible con la ROM | 34% |

El aumento de correspondencia significa que ya existe una matriz completa de
casos verificables. No significa que se hayan incorporado al repositorio
capturas de referencia ni que todos los casos coincidan todavía con Mesen.

## Catedral B-Type completada

- Estado incremental para diez niveles y seis cantidades de sprites.
- Velocidad de animación y demora de movimiento desde las tablas originales.
- Alternancia exacta entre los dos metasprites de cada nivel.
- Posición inicial, sentinel, vector X, disparadores y coordenadas Y desde PRG.
- Orden 6502 respetado: seleccionar sprite, incrementar demora, dibujar posición
  anterior, mover y activar el siguiente objeto.
- Activación en cascada dentro del mismo fotograma cuando corresponde.
- Compatibilidad con el renderer de acceso aleatorio ya utilizado por SDL.
- Prueba exhaustiva de 43,200 estados: 10 niveles × 6 alturas × 720 fotogramas.

## Matriz APU completa

`tools/apu_matrix.py` define 18 casos:

- pistas originales 1 a 10;
- movimiento;
- rotación;
- bloqueo;
- línea;
- Tetris;
- subida de nivel;
- derrota;
- final completado.

La orden `capture` ejecuta `tetris_apu_render` y `tetris_apu_scenario`, crea las
trazas y guarda un manifiesto JSON con hashes, cantidad de fotogramas, ciclos y
fotogramas con escrituras. La orden `compare` localiza el primer caso,
fotograma y columna divergente contra una matriz de referencia generada por el
usuario.

## Validación de la fase

- Autoprueba de matriz con casos iguales y divergentes.
- Autoprueba de los ocho escenarios de efectos.
- Prueba exhaustiva de la máquina de catedral.
- Compilación estricta del nuevo ejecutable de escenarios en MSVC y GCC/Clang.
- Windows, Linux y Android deben terminar en verde antes de fusionar.
- El APK rechaza ROM, replay, OGG, WAV y CSV incrustados.

## Diferencias conocidas

- Las escrituras APU siguen aplicándose a granularidad de instrucción y no al
  ciclo de bus exacto.
- La demora real de `$4017` y la entrega asíncrona de IRQ siguen simplificadas.
- La matriz necesita capturas Mesen generadas localmente para medir igualdad.
- La demo usa tablas originales, pero sus reglas principales se ejecutan en C.
- Aún no existe una construcción 6502 enlazable o binariamente idéntica.

## Próxima fase hacia exactitud

1. Añadir microtemporización de lecturas y escrituras APU dentro de cada opcode.
2. Implementar la demora exacta de `$4017` y la entrada IRQ de siete ciclos.
3. Ejecutar la matriz contra Mesen y corregir la primera divergencia de cada
   familia de pistas/efectos.
4. Continuar la comparación de RAM de la demo y corregir su primera divergencia.
5. Etiquetar y traducir más rutinas PRG.
6. Preparar una construcción 6502 enlazable y después perseguir identidad
   binaria.
