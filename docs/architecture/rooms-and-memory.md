# Salas, cargador y memoria

Rutas relativas a `src/ps2/veronica/prog/`.

## Memoria

Toda la memoria del juego son arrays estáticos en BSS. No hay direcciones absolutas, así que el ejecutable puede crecer.

| Bloque | Dónde | Tamaño | Uso |
| --- | --- | --- | --- |
| `Ps2_malloc_mem` | ps2_sg_maloc.c | 13.422.592 B | Arena de `syMalloc`/`syFree` |
| `freemem` | `syMalloc(12845056)` en main.c:136 | 12.25 MB | Pool del juego. La parte alta son los buffers de vértices del Ninja (`njpmemp`, `vwbmemp`, `vebmemp`); `sys->endp` marca su inicio |
| `Ps2_tex_mem` | ps2_dummy.c:35 | 10 MB | Memoria de texturas (`Ps2TextureMalloc`, ps2_NaTextureFunction.c:552), con 256 entradas `tbuf` |

### Pool del juego: asignador lineal

`bhGetFreeMemory(size, align)` (pwksub.c:16) avanza `sys->memp` y devuelve el bloque. No hay `free`. La estructura del pool es esta:

```text
freemem
├─ copia de la partida para reintentar (tamaño = save_end − version)   bhInitSystem, system.c:128
├─ memoria de la máquina de escribir                                    TypewriterKeepMemory
├─ keepmem
├─ buffers fijos del jugador (~724 KB)                                  bhInitPlayer, player.c:639-653
├─ sys->mempb  ◄── marca: todo lo de debajo sobrevive a los cambios de sala
├─ datos de la sala actual (modelos, enemigos, efectos, PP_WORK...)     se asignan avanzando sys->memp
└─ ... hasta sys->endp
```

**Al cambiar de sala**, el cargador hace `sys->memp = sys->mempb` (system.c:1523): todo lo asignado después de `mempb` se pierde. Si hace falta memoria persistente nueva (por ejemplo, para un segundo jugador), hay que reservarla antes de que `bhInitPlayer` fije `mempb` (player.c:659), o usar arrays estáticos.

`bhGetFreeMemory` imprime "malloc area = %x". Sirve para medir el margen de memoria en la consola de PCSX2.

## `SYS_WORK` y la partida guardada

`SYS_WORK` (types.h:545, 0x2ACF0 bytes, global `sys`) es el estado global del juego. **No hay que cambiar su estructura**:

- El rango desde `version` hasta `save_end` se copia en bruto:
  - en la tarjeta de memoria (ps2_McSaveFile.c:119, :178);
  - en la instantánea para reintentar (`bhPushGameData`/`bhPopGameData`, room.c:1033/1043);
  - en los datos de demo (system.c:1506).
- `bhInitSystem` pone a cero 175.260 bytes a partir de `tk_flg`, un tamaño escrito a mano (system.c:124).

Si se necesita estado nuevo, se pone en variables globales aparte.

## Datos de la sala: `ROM_WORK` (`rom`)

`rom` (types.h:1341) apunta a las tablas de la sala cargada (fichero RDT):

| Campo | Contenido |
| --- | --- |
| `cutp` / `cut_n` | Planos de cámara y sus zonas |
| `lgtp` / `lgt_n` | Luces. `lgtp[1]` es la luz del mechero del jugador |
| `enep` / `ene_n` | Enemigos y NPCs de la sala (`ETTY_WORK`: id, tipo, posición…) |
| `objp`, `itmp` | Objetos e items |
| `walp`, `etcp`, `flrp` | Paredes; zonas de examinar/puerta/escalera/item; zonas de suelo |
| `posp` / `pos_n` | Puntos de aparición del jugador, que se eligen con `sys->pos_no` |
| `rutp`, `ruttp` | Rutas para la IA |
| `evtp`, `evcp`, `mesp`, `evlp` | Scripts de evento, cámaras de evento, mensajes, luces de evento |
| `grand[32]` | Altura de cada piso: `rom->grand[flr_no + 2]` |
| `mdl` | Modelo de la sala |
| `amb_*`, `fog*` | Ambiente y niebla |

## El cargador: `bhSysCallMonitor` (system.c:1305)

Es la tarea 20. Es una máquina de estados con `sys->mn_mode0..3`, que se asignan con la macro `SET_SYS_MN_MODE(m0, m1, m2, m3)` (macros.h). Si se pide un modo mientras hay otro en curso, se apila en `sys->mn_stack` (system.c:1315).

| `mn_mode0` | Qué hace |
| --- | --- |
| 1 | **Carga inicial** (partida nueva o cargada). Paso 4: `bhInitObjItm`, `bhInitEffect`, `bhInitCamera`, `bhInitPlayer`, `bhInitEnemy`. Pasos 6-7: fichero de personaje y `bhReadPlayerData`. Pasos 8-9: fichero de arma y `bhReadWeaponData`. Paso 10: pasa a modo 4 |
| 2 | **Cambio de sala**: paso 0 hace `memp = mempb`; después pasa a modo 4 |
| 3 | Sin documentar todavía |
| 4 | **Carga de sala**: ver abajo |

### Carga de sala (modo 4)

1. Lee `rm_<stage><room:2><rcase>.rdx` (system.c:1623), por ejemplo `rm_0000.rdx` para el stage 0, sala 0, caso 0.
2. `bhInitReadRDT` (room.c:28):
   - `bhInitRoomChangeSystem` (system.c:179) pone a cero las paredes, zonas y suelos dinámicos y **el estado del mando**;
   - `bhInitCamera`;
   - `bhInitEnemy` **borra `ene[]`**;
   - `bhInitRoom`;
   - `bhClearEffect` borra los efectos, **incluida la sombra del jugador**.
3. `Expand` (descompresión), y después `bhSetRDT` → `bhSetRoom` (room.c:47). Este crea los enemigos de `rom->enep` (`bhSetEneMdl` room.c:559 → `bhSetEnemy`), los objetos (`sys->obwp[4..]`) y los items (`sys->itwp`).
4. Texturas.
5. **`bhFinishRoom`** (room.c:337), que se llama en system.c:1723:
   - carga las animaciones de los enemigos (`bhSetEneMtn` → `sys->emtp[id]`);
   - coloca al jugador (room.c:477-520);
   - en partida nueva pone `mode0 = 0`; si no, llama a `bhInitRoomChangePlayer`;
   - pone `sp_flg = 0xFFFFFFFF` (todo activo).
6. Si hay un cambio de personaje pendiente (`cb_flg & 0x80`), los pasos 4-7 recargan modelo y arma (system.c:1725-1810).

**Qué sobrevive a un cambio de sala:** `ply` y sus buffers (están por debajo de `mempb`), `SYS_WORK` y los objetos `obwp[0..3]` del jugador.

**Qué se pierde:** `ene[]`, `eff[]` (sombras incluidas), `obwp[4..]`, `itwp`, las zonas dinámicas (`mwalp`, `metcp`, `mflrp`) y todo lo asignado por encima de `mempb`.

### Puertas

1. El botón de acción sobre una zona de puerta (`bhCheckExmAtari`, tipo 0) rellena `sys->door` y pone `cb_flg |= 1`. Los scripts también pueden abrir puertas con `bhSetDoorCall` → `bhSetDoorDemo` (room.c:883).
2. `bhMainSequence` llama a `bhStartDoorDemo` (room.c:940): fundido, fija `sys->stg_no/rom_no/pos_no` y `mn_mode0 = 2`.
3. La animación de la puerta corre en la tarea 11 (`bhSysCallDoordemo`, door.c) mientras se carga la sala nueva.
