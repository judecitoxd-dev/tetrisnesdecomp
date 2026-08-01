# Formato de repetición TTRPLY01

La versión 0.5 guarda automáticamente la última partida en `last_replay.ttr`. El archivo contiene
solo configuración de partida, entradas del jugador y un hash del estado final. No contiene ROM,
gráficos, música ni otros recursos del cartucho.

## Objetivos

- reproducir una partida de forma determinista;
- detectar cambios involuntarios en la lógica;
- permitir pruebas sin vídeo ni audio;
- mantener el archivo pequeño y fácil de validar.

## Encabezado

Todos los enteros se escriben en orden little-endian.

| Campo | Tamaño | Descripción |
|---|---:|---|
| Magic | 8 bytes | ASCII `TTRPLY01` |
| Semilla | 4 bytes | Semilla inicial del núcleo |
| Modo | 1 byte | `0` A-Type, `1` B-Type |
| Nivel | 1 byte | Nivel inicial 0–19 |
| Altura | 1 byte | Altura B-Type 0–5 |
| NEXT inicial | 1 byte | `0` oculto, `1` visible |
| Fotogramas | 4 bytes | Número de entradas, máximo 1,000,000 |
| Hash final | 8 bytes | Hash determinista de 64 bits |

Después del encabezado aparecen `fotogramas` palabras de 16 bits. Solo se usan nueve bits:

| Bit | Entrada |
|---:|---|
| 0 | izquierda mantenida |
| 1 | derecha mantenida |
| 2 | abajo mantenido |
| 3 | rotación horaria pulsada |
| 4 | rotación antihoraria pulsada |
| 5 | caída instantánea pulsada |
| 6 | pausa pulsada |
| 7 | reinicio pulsado |
| 8 | mostrar/ocultar NEXT pulsado |

El cargador rechaza firmas o versiones desconocidas, valores fuera de rango, conteos excesivos,
archivos truncados y bytes sobrantes.

## Hash de estado

`tetris_state_hash()` serializa explícitamente los campos deterministas del juego y aplica una
variante estable de FNV de 64 bits. No se calcula sobre la memoria cruda de la estructura, evitando
que padding, arquitectura o compilador alteren el resultado. Los eventos transitorios ya consumidos
no forman parte del hash.

El hash confirma que una repetición terminó en el mismo estado; no prueba que todos los estados
intermedios coincidieron. Una fase posterior puede guardar checkpoints periódicos.

## Verificación

```bash
./tetris_replay_verify last_replay.ttr
```

Salida de ejemplo:

```text
frames=1800 expected=c83327fd4001e568 actual=c83327fd4001e568
```

Códigos de salida:

- `0`: coincidencia;
- `1`: desincronización;
- `2`: argumento o archivo no válido.

## Compatibilidad

La firma incluye el número de formato. Una modificación incompatible debe usar una firma/versión
nueva en vez de reinterpretar silenciosamente archivos existentes.
