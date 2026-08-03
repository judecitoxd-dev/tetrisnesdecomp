# Verificación semántica del PRG 6502

La versión 0.23 añade `tools/prg_verify.py` y
`tools/tetris_prg_manifest.json`. El verificador trabaja únicamente con la ROM
legal proporcionada por el usuario y no exporta PRG, CHR, audio ni capturas.

## Qué comprueba

- CRC32 y SHA-256 del archivo, PRG y CHR;
- vectores NMI, RESET e IRQ;
- 53 firmas de entrada asociadas a rutinas con nombre semántico;
- 9 tablas completas, incluida la tabla de 19 orientaciones;
- 14 aristas JSR del flujo de control;
- gravedad NTSC, columnas de borrado, apariciones, basura y puntuación BCD.

En total se verifican 1,136 bytes mediante firmas o hashes de tablas. El hash
del PRG completo confirma además que se está analizando la revisión esperada,
pero no se contabiliza como 100% decompilado: conocer el hash no equivale a
comprender, etiquetar y reconstruir cada rutina.

## Uso

```bash
python tools/prg_verify.py "Tetris (USA).nes" --report prg-report.json
python tools/disassemble_prg.py "Tetris (USA).nes" --aggressive \
  --symbols tools/tetris_symbols.json --report disassembly-report.json
```

El informe contiene solo hashes, cantidades y resultados. No contiene bytes de
la ROM.

## Reglas y timing corregidos en 0.23

- `DAS_DELAY=$0A` y `DAS_RESET=$10` para NTSC;
- la carga horizontal se conserva durante ARE y borrado de líneas;
- mantener DOWN no descarga el DAS horizontal;
- RIGHT tiene prioridad cuando LEFT y RIGHT están pulsados simultáneamente;
- un movimiento bloqueado carga el contador hasta `DAS_RESET`;
- borrado de líneas y cortina de derrota se sincronizan con el contador global
  de frames, como las rutinas `$977F` y `$9A11`.

## Límite de la cifra de progreso

Una firma correcta demuestra que una dirección corresponde a la revisión y a
la rutina documentada. No demuestra por sí sola equivalencia de todos sus
estados internos. Por eso la cifra de decompilación sigue siendo una estimación
conservadora y separada de la cobertura heurística del desensamblador.
