# Herramientas de desensamblado y espacio de trabajo

Estas herramientas avanzan la investigación del PRG 6502 sin almacenar la ROM ni bytes extraídos en
el repositorio. Los archivos derivados deben permanecer en el equipo del usuario.

## Símbolos conocidos

`tools/tetris_symbols.json` describe vectores, tablas, rangos de datos y rutinas identificadas con
alta confianza. Entre ellas están las entradas NMI/RESET/IRQ, gravedad, borrado, paletas, selector de
pieza, LFSR, lectura de mandos y DMA de sprites.

El archivo es deliberadamente declarativo para que los nombres y rangos puedan revisarse sin editar
el desensamblador.

## Desensamblador recursivo

`tools/disassemble_prg.py` implementa los 151 opcodes oficiales del NMOS 6502 y modela la ROM como
dos bancos PRG de 16 KiB:

- banco 0: ventana CPU `$8000-$BFFF`;
- banco 1 fijo: ventana CPU `$C000-$FFFF`.

Empieza desde vectores y símbolos conocidos, sigue saltos, llamadas y flujo secuencial, evita rangos
marcados como datos y genera etiquetas/referencias cruzadas.

```bash
python tools/disassemble_prg.py --self-test
python tools/disassemble_prg.py "Tetris (USA).nes" \
  --symbols tools/tetris_symbols.json \
  --output tetris_recursive.asm \
  --report tetris_recursive.json \
  --dot tetris_recursive.dot
```

### Modo agresivo

```bash
python tools/disassemble_prg.py "Tetris (USA).nes" --aggressive \
  --output tetris_aggressive.asm --report tetris_aggressive.json
```

El modo agresivo añade como raíces destinos plausibles observados en bytes que parecen `JSR`/`JMP`.
Aumenta cobertura, pero puede interpretar datos como instrucciones. Sus números son una ayuda de
triage, no un porcentaje de decompilación verificada.

## Espacio de trabajo privado

`tools/create_decomp_workspace.py` crea una carpeta con:

- banco PRG 0 y banco PRG 1;
- CHR extraído;
- listados conservador y agresivo;
- informes JSON y grafos de llamadas;
- manifiesto de hashes;
- README con advertencia legal.

```bash
python tools/create_decomp_workspace.py "Tetris (USA).nes" mi_decomp_privado
```

La herramienta se niega a sobrescribir una carpeta no vacía salvo que se use `--force`.

**No subas el resultado al repositorio.** Los `.bin` y listados pueden contener material extraído de
la ROM. Solo deben usarse localmente con una copia que tengas derecho a analizar.

## Cobertura actual de la ROM comprobada

- recorrido conservador: 1,166 instrucciones, 2,441 bytes de código, 7.45% del PRG;
- recorrido agresivo: 3,839 instrucciones, 7,818 bytes, 23.86% del PRG.

La segunda cifra incluye hipótesis. El objetivo siguiente es promover raíces heurísticas a símbolos
confirmados mediante trazas de emulador, referencias cruzadas y comparación de comportamiento.

## Flujo recomendado

1. Ejecutar el recorrido conservador.
2. Revisar llamadas salientes desde rutinas ya nombradas.
3. Usar el listado agresivo para encontrar candidatos.
4. Confirmar candidatos con un depurador/emulador y añadirlos a `tetris_symbols.json`.
5. Marcar tablas/rangos de datos para impedir desensamblado falso.
6. Traducir una rutina a C y añadir una prueba de equivalencia observable.
7. Mantener separados “identificado”, “traducido” y “reproducible”.
