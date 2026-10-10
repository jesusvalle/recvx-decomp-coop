# Salas, cargador y memoria

Rutas relativas a `src/ps2/veronica/prog/`.

## Memoria

Toda la memoria del juego son arrays estáticos en BSS. No hay direcciones absolutas, así que el ejecutable puede crecer.

| Bloque | Dónde | Tamaño | Uso |
| --- | --- | --- | --- |
| `Ps2_malloc_mem` | ps2_sg_maloc.c | 13.422.592 B | Arena de `syMalloc`/`syFree` |
| `freemem` | `syMalloc(12845056)` en main.c:136 | 12.25 MB | Pool del juego. Lo útil es `[freemem, sys->endp)` = 12.255.232 B (11,69 MB). Por encima de `endp` (= `njpmemp`) están los buffers del skinning (`np.buff`, 2×128 KB, njplus.c:15-16) y los de vértices del Ninja (`vwbmemp` = `freemem` + 12.517.376) (main.c:136-142; system.c:54-57, 114, 130) |
| `Ps2_tex_mem` | ps2_dummy.c:35 | 10 MB | Memoria de texturas (`Ps2TextureMalloc`, ps2_NaTextureFunction.c:552), con 256 entradas `tbuf` |

### Pool del juego: asignador lineal

`bhGetFreeMemory(size, align)` (pwksub.c:16) avanza `sys->memp` y devuelve el bloque. No hay `free`. La estructura del pool es esta:

```text
freemem
├─ copia de la partida para reintentar (2.100 B = save_end − version)   bhInitSystem, system.c:128
├─ memoria de la máquina de escribir (800 B)                            TypewriterKeepMemory
├─ keepmem
├─ sys->obwp + sys->itwp (2 × 39.936 B)                                 system.c:450
├─ sysmes.ald (89.088 B) y doordp (172.032 B)                           modo 1 del cargador
├─ modelos de efectos de SYSTEM.AFS[1] (~9 KB)                          effect.c:42-110
├─ buffers fijos del jugador (741.632 B)                                bhInitPlayer, player.c:639-653
├─ (build COOP) memoria de P2 (65.792 B)                                coopInitMemory
├─ sys->mempb  ◄── marca: todo lo de debajo sobrevive a los cambios de sala (medido con COOP del hito 2a en la sala 0-1: `mempb − freemem` ≈ 1,16 MB; `endp − mempb` = 11.094.656 B)
├─ datos de la sala actual (modelos, enemigos, efectos, PP_WORK...)     se asignan avanzando sys->memp
├─ ...
├─ Ps2_PXLCONV: últimos 320 KB por debajo de endp                       main.c:140; zona de trabajo de cada carga de TIM2 (ps2_pxlconv.c:15)
└─ sys->endp
```

**Al cambiar de sala**, el cargador hace `sys->memp = sys->mempb` (system.c:1523): todo lo asignado después de `mempb` se pierde. Si hace falta memoria persistente nueva (por ejemplo, para un segundo jugador), hay que reservarla antes de que `bhInitPlayer` fije `mempb` (player.c:663), o usar arrays estáticos. `sys->lmmdlp` (32 KB, por debajo de `mempb`) no lo usa el juego; desde el hito 2a lo usa coop.c (buffers de la coleta de P2 y huesos de sus manos).

**`bhGetFreeMemory` solo comprueba el tamaño total del pool (12.845.056, pwksub.c:22), no `endp`**: puede pisar `np.buff` sin devolver `NULL`. Además, casi ningún llamador comprueba el `NULL`.

`bhGetFreeMemory` imprime "malloc area = %x" (pwksub.c:33). El primer mensaje (máquina de escribir) vale `freemem` + 2.912, lo que permite calcular `freemem`. Con el SDK 2.0 estos `printf` no salen en el log de PCSX2.

### Margen por sala (tamaños de las 205 RDX de la ISO + valores medidos en juego con `ramread.py`)

- Una sala se lee expandida en `memp` (máximo 8.931.872 B en RM_0030, mediana 6,22 MB). Al final de `bhSetRoom` se hace `sys->memp = reladr` (room.c:337), que recupera la sección de texturas. Lo que queda (la parte `hed04`) es como mucho 2.915.836 B (RM_9350), con mediana 1,37 MB.
- Durante la carga hace falta `mempb + sala expandida ≤ endp − 320 KB`. Con `mempb` medido (hito 2a, con P2), en RM_0030 sobran unos 1,83 MB; en la sala mediana, unos 4,4 MB.
- Otras reservas en partida que salen del mismo espacio: `bhEff135`, 2 MB (effsub1b.c:633); el inventario, 128 KB (sub1.c:1562); la copia de las texturas de la sala que hacen el mapa y el inventario (map.c:131); y el cambio de personaje, que lee 1,29 MB en `memp`.
- Pool de texturas: efectos de `SYSTEM.AFS[1]` (1.090.304 B) + Claire (0,27 MB) + arma + sala. La sala con más textura es RM_4030 (7,39 MB): el total ronda los 8,8 MB de 10 MB. Ver el sistema de texturas en [world-systems.md](world-systems.md).

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
| 1 | **Carga inicial**. Paso 4: `bhInitObjItm`, `bhInitEffect`, `bhInitCamera`, `bhInitPlayer`, `bhInitEnemy`. Pasos 6-7: fichero de personaje en `ALIGN_UP(memp, 64)` y `bhReadPlayerData` (system.c:1426-1445). Pasos 8-9: fichero de arma y `bhReadWeaponData` (:1446-1470). Paso 10: pasa a modo 4 (:1471-1493). Pasos 20-21: demo. Es la **única** carga completa: partida nueva, cargar partida, reintentar (`bhPopGameData` → `bhExitGame` → `bhFirstGameStart`, room.c:1045-1066) y Battle Game entran por `bhFirstGameStart` (system.c:466). Antes de cada modo 1 se liberan todas las texturas: `bhExitGame` (system.c:2441, `njReleaseTextureAll`), la vuelta al título (adv.c:913) y la máquina de escribir (bup_00.c:427). La rama del paso 10 con `gm_flg & 0x400000` (`bhResetPlayer`) está muerta: ningún código pone ese bit, y `bhInitRoomChangeSystem` lo borra (system.c:189) |
| 2 | **Cambio de sala**: paso 0 hace `memp = mempb`; después pasa a modo 4. No recarga el personaje salvo con `cb_flg & 0x80` (cambio de personaje, system.c:1768-1817) |
| 3 | **Cambio de arma** desde el inventario: lee el fichero de arma y llama a `bhReadWeaponData` (system.c:1573-1608). Ver [inventory.md](inventory.md) |
| 4 | **Carga de sala**: ver abajo |
| 5 | **Abrir el inventario** (system.c:1962-2036). Ver [inventory.md](inventory.md) |
| 6 | Modelo 3D del objeto que se examina en el inventario (itemview.c:284-315) |

**Lectura de ficheros:** asíncrona y por un único canal, compartido con el sonido (sdfunc.c:3053-3133).

- `GetInsideFileSize(sys->sys_partid, id)` es síncrono (abre y cierra el fichero).
- `RequestReadInsideFile(part, id, dst)` devuelve -1 si ya hay otra lectura en curso. El juego ignora ese valor.
- `GetReadFileStatus()` devuelve 1 mientras lee, 0 al terminar y -1 si hay error (bandeja abierta o reset).
- `ExecFileManager` avanza la lectura en cada `ExecSoundSynchProgram` (sdfunc.c:484).

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
