# Mapa técnico de la ROM comprobada

Este documento registra las partes pequeñas del PRG que ya se identificaron y se
tradujeron a la reimplementación C. Los offsets se verifican con la ROM del usuario
y no requieren incluir datos extraídos en el repositorio.

## Cabecera y vectores

| Elemento | Valor |
|---|---:|
| Formato | NES 2.0 |
| Mapper | MMC1 / 1 |
| PRG | 32 KiB |
| CHR | 16 KiB |
| NMI | `$8005` |
| RESET | `$FF00` |
| IRQ | `$804A` |

## Tablas localizadas

| Tabla | Offset PRG | Dirección CPU | Uso en el port |
|---|---:|---:|---|
| Gravedad NTSC | `0x098E` | `$898E` | `GRAVITY_FRAMES` |
| Columnas de borrado | `0x17FE` | `$97FE` | Animación centro hacia afuera |
| Orientaciones iniciales | `0x194E` | `$994E` | Rotación de aparición por pieza |
| Puntuación BCD | `0x1CA5` | `$9CA5` | 40, 100, 300 y 1200 |
| Paletas por nivel | `0x184C` | `$984C` | Identificada; todavía no conectada al render RGB |

## Lógica ya traducida

- Las 19 orientaciones de tetrominós usan el mismo pivote y desplazamientos del
  programa 6502.
- El selector de piezas usa el LFSR de dos bytes, contador de apariciones y una
  segunda elección cuando aparece el índice inválido o se repite la pieza.
- La gravedad utiliza los 30 bytes NTSC del cartucho.
- La subida de nivel reproduce la comparación BCD del original, incluyendo las
  transiciones especiales de niveles iniciales 10–19.
- El borrado oculta pares de columnas `4/5`, `3/6`, `2/7`, `1/8`, `0/9`, un paso
  cada cuatro fotogramas.

## Pendiente

Todavía falta etiquetar rutinas completas de menús, audio, finales y renderizado,
convertir las paletas NES a RGB desde las tablas del PRG y crear pruebas de estado
por fotograma contra un emulador de referencia.

Ejecuta lo siguiente para verificar los offsets con tu copia legal:

```bash
python tools/rom_tables.py "Tetris (USA).nes"
```
