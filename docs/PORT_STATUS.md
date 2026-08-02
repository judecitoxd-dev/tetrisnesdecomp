# Estado de decompilación y ports

## Resumen de v0.13

La decimotercera fase sincroniza el intérprete 6502 y el APU con el reloj NTSC.
Cada instrucción oficial aporta sus ciclos base, las ramas y lecturas indexadas
aplican sus penalizaciones y el APU avanza con esos ciclos en lugar de estimarse
únicamente desde la cantidad de muestras.

El controlador original continúa ejecutándose una vez por fotograma, pero sus
escrituras afectan ahora temporizadores, envolventes, secuenciador, DMC e IRQ en
una línea temporal de CPU medible.

## Progreso estimado

| Área | Estado aproximado |
|---|---:|
| Port nativo jugable básico | 99% |
| Port Android jugable | 97% |
| Controles, tablero y puntuación | 99% |
| Modo A | 96% |
| Modo B | 92% |
| Integración y empaquetado | 99% |
| Carga legal de recursos desde ROM | 99% |
| Fidelidad de reglas y timings principales | 86% |
| Pantallas y animaciones originales/equivalentes | 94% |
| Audio original interactivo | 95% |
| Renderizado automático del APU original | 90% |
| Decompilación etiquetada/verificada del PRG 6502 | 40% |
| Correspondencia reproducible con la ROM | 25% |

El 90% del APU representa una implementación funcional, interactiva y guiada
por ciclos. El porcentaje restante exige colocar cada lectura/escritura en su
ciclo de bus exacto, entregar IRQ al flujo normal de CPU y demostrar igualdad
de trazas para todas las pistas y efectos.

## Implementado en v0.13

- Conteo de ciclos para las instrucciones oficiales soportadas.
- Penalización de un ciclo por rama tomada y otro por cruce de página.
- Penalizaciones de cruce de página para lecturas indexadas.
- Callback de ciclos desde el 6502 hacia el APU.
- Distribución de aproximadamente 29,780.5 ciclos por fotograma NTSC.
- Temporizadores de pulso y ruido a la mitad del reloj de CPU.
- Temporizador de triángulo a reloj completo.
- Temporizador DMC según la tabla NTSC.
- Secuenciador APU de cuatro y cinco pasos por ciclos.
- Quarter frame y half frame para envolventes, longitudes, barridos y contador
  lineal.
- Frame IRQ y DMC IRQ visibles en `$4015`.
- Lecturas DMC con robos de cuatro ciclos contabilizados durante el driver.
- Mezcla de muestras mientras transcurren los ciclos libres del fotograma.
- Medición de ciclos totales, ciclos del driver y stalls en las trazas CSV.
- Implementación portable para GCC, Clang, MSVC y Android NDK.

## Validación de la fase

- Prueba de `LDX`, lectura indexada con cruce de página, rama tomada, `NOP` y
  `RTS`: 18 ciclos.
- Prueba de secuenciador: decremento de longitud al half frame.
- Prueba de frame IRQ: activación, lectura por `$4015` y limpieza.
- Prueba DMC: lectura de muestra, stall de cuatro ciclos, IRQ y limpieza.
- Fotograma artificial: 798 muestras, 29,780 ciclos y audio no silencioso.
- El PR debe pasar Windows, Linux y Android antes de fusionarse.

## Diferencias conocidas

- Las escrituras de una instrucción se aplican antes de avanzar todos sus ciclos;
  todavía no se ubican en el ciclo de bus exacto de la instrucción.
- La demora de tres/cuatro ciclos de `$4017` sigue simplificada.
- Frame IRQ y DMC IRQ se exponen, pero el driver aislado no ejecuta un handler
  asíncrono entre llamadas de fotograma.
- Los stalls DMC se contabilizan durante instrucciones; durante los ciclos
  ociosos no existe CPU útil que detener.
- Falta validar automáticamente todas las pistas, variantes allegro y efectos
  contra Mesen.
- La demo usa entradas y piezas originales, pero las reglas principales siguen
  ejecutándose en C.
- Falta completar la entrada/movimiento de la catedral B-Type.
- No existe una construcción 6502 enlazable o binariamente idéntica.

## Próxima fase hacia exactitud

1. Añadir microtemporización de bus para lecturas y escrituras APU.
2. Implementar la demora real de `$4017` y bordes exactos del secuenciador.
3. Entregar IRQ al intérprete y modelar su entrada de siete ciclos.
4. Automatizar una matriz de diez pistas, allegro y efectos contra Mesen.
5. Corregir la primera escritura APU divergente hasta igualdad de traza.
6. Continuar la comparación de RAM de la demo y corregir su primera divergencia.
7. Completar la máquina de movimiento de la catedral B-Type.
8. Ampliar la traducción del PRG y preparar una construcción 6502 enlazable.
