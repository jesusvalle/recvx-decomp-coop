# Inventario, baúles y pantalla de estado

Rutas relativas a `src/ps2/veronica/prog/`. Los números de línea son del árbol de trabajo con los ganchos del cooperativo (octubre de 2026): en system.c están unas 8 líneas por debajo de los de documentos anteriores. Si no cuadran, busca por el nombre de la función. Lo deducido va marcado como "deducido" y lo no comprobado como "sin confirmar".

## `sys->itm[384]`

- 384 `u32` en el offset 0x270 de `SYS_WORK` (types.h:593), **dentro** del rango guardado `version`..`save_end` (types.h:576, 606).
- `SAVEFILE` lleva `itm` entero con checksum (types.h:1497-1534; ps2_McSaveFile.c:178-182). Por tanto `itm` viaja en la tarjeta de memoria, en la instantánea de reintento y en los datos de demo.

### Formato de una entrada

`(flags << 24) | (id << 16) | cantidad`

| Bits | Qué es | Dónde |
| --- | --- | --- |
| 0-15 | Cantidad o munición (tope 999 si es apilable) | `GetItem`, sub1.c:3772-3799, 4014-4024 |
| 16-23 | Id de objeto: índice en `itemdata[156]` | item.c:109 |
| `0x08000000` | Cantidad o munición infinita | weapon.c:430, 448; sub1.c:4035; ps2_McSaveFile.c:153 |
| `0x10000000`, `0x20000000`, `0x40000000` | Tipo de munición cargada en armas con varias municiones | `WeaponSet` sub1.c:4489-4515; `bhSearchBullet` sub1.c:8887-8892 |
| `0x80000000` | Estado propio del objeto (por ejemplo, id 0x48 examinado) | itemview.c:720; sub1.c:8356, 8390 |

Tipos de `itemdata[].type`: `0x1` arma equipable, `0x2` munición, `0x8` curación, `0x40` objeto clave, `0x100` ocupa dos casillas, `0x200` apilable, `0x3800` muestra contador (sub1.c:4608, 8900, 2683, 2643).

### Bloque de un personaje: `&sys->itm[ply_id*16]`

| Entrada | Contenido |
| --- | --- |
| `[0]` | Índice de la casilla equipada (0 = nada): `WeaponSet` sub1.c:4562-4579; `bhCheckBullet` weapon.c:426-428 |
| `[1]` | Casilla especial fija que se dibuja aparte (sub1.c:2453-2497). El mechero (id 55, `pip[1] = 0x370001`, sub1.c:8503) va siempre ahí (sub1.c:4078-4083); `cb_flg 0x1000000` la sustituye (sub1.c:4141-4161) |
| `[2..9]` | 8 casillas, en rejilla de 2 columnas (sub1.c:2501-2502) |
| `[10..11]` | 2 casillas más con la mochila (`gm_flg 0x8000000`, sub1.c:1747, 2357, 7186) |
| `[12..15]` | Sin uso como objetos. `[12]` lo leen los desplazamientos de `bhPlItemLost`/`bhPlItemLostEx` (event.c:1334-1337, 8294-8297), así que debe valer 0 |

`ItemSort` (sub1.c:2284-2342) ordena `[2..11]`; `ItemSearch` (sub1.c:9013-9023) busca en `[1..11]`.

### Rangos

| Rango | Qué es |
| --- | --- |
| 0..15 | `ply_id` 0, Claire. Partida nueva: mechero en `[1]` y el id 145 (visor de archivos) en `[2]` (`AllItemInit`, sub1.c:8499-8521) |
| 16..31 | `ply_id` 1, Chris (deducido) (sub1.c:8523-8537) |
| 32..47 | `ply_id` 2. `AllItemInit` lo inicializa con arma equipada: `[0]=2`, `[1]=id 138`, `[2]=id 34 ×400` (sub1.c:8539-8547). Quién es (¿Steve?) y si se usa en la historia: sin confirmar |
| 48..63 | `ply_id` 3. En la historia no se inicializa; solo `ExtraGameItemInit` (Battle Game) |
| 64..191 | Baúl general, 128 entradas, índice `& 0x7F` (sub1.c:2632-2640, 8257-8292, 7490-7505) |
| 192..223 | Baúl especial A (`cb_flg 0x80000`). `bhItemPlToSBox` vacía 192..255 y mueve ahí los objetos de un personaje (event.c:7936-7945) |
| 224..255 | Baúl especial B (`cb_flg 0x100000`). También almacén de `bhPlItemLostEx` (`224 + v1`, event.c:8273); el byte bajo de `itm[224]` guarda la cantidad del id 4 (sub1.c:6564, 4074-4076) |
| **256..382** | **Sin referencias en el código**: solo `AllItemInit` lo pone a cero. Libre |
| 383 | Bits de archivos leídos (fileview.c:128, 351; itemview.c:1201-1227) |

Riesgo sobre 256..382: varios opcodes de script calculan el índice con operandos del guion (`bhItemGetGet` 0xB2 `itm[v0*16+2]`, event.c:7759; `bhItemGetGetEx` 0xC2; `bhItemPlToSBox` 0xB7 `v0*16`; `bhPlItemLostEx` 0xBF `224+v1`). Solo llegarían a 256 o más con `v0 ≥ 16` o `v1 ≥ 32`; no se han escaneado los RDX para comprobarlo. Además, si el baúl general está lleno, el bucle de `bhItemSBoxToIBox` (0xB8, event.c:7957-7971) puede pasar de 192.

### Otros datos por personaje

- `sys->ply_hp[4]`, `ply_wno[4]` y `ply_stflg[4]`. Se escriben en la partida nueva (system.c:487-494), en `bhPushGameData` (room.c:1030-1039), en la máquina de escribir (bup_00.c:364-374), en el cambio de personaje (system.c:1737, 1754-1755) y desde scripts (event.c:830, 1297, 7421, 7452, 7773, 7784). Solo se leen con índice `sys->ply_id` (player.c:671-675, 1001-1004; room.c:487, 501; system.c:1795).
- **Mochila:** `bhCheckSubPack` (player.c:1080-1117) pone `gm_flg 0x8000000` según `ev_flg` 6/7/8 para `ply_id` 0/1/2. Es un único bit para el personaje actual. En el Battle Game está siempre activa (sub1.c:3392-3395).
- `standard[3][2]` (sub1.c:1287): objetos que no se guardan en el baúl, por `ply_id`: `{55, 50}` para Claire (55 = mechero; 50 sin confirmar), `{55, -1}`… Solo tiene 3 filas.

## Abrir el inventario

1. **Botón:** bit lógico `0x4000`, que en la configuración A sale de la máscara física `0xC00` (pad.c:8): Start o R3 (deducido). Start también genera `0x20000`, y L1+Start es la pausa; por eso el inventario exige `!(pad_on & 0x80)`.
2. **`bhCheckSubTask`** (system.c:568-622), llamada desde la tarea Game.
   - Guarda exterior: no hay pausa (`!(ss_flg & 0x80000000)`), ninguna subpantalla (`!(st_flg & 0x1C040008)`), ni puerta ni fundido (`!(cb_flg & 0x3)`), y no hay vídeo (`ts_flg & 0x1000`).
   - No se abre si el jugador está agarrado o golpeado (`plp->flg & 0x6`, system.c:572).
   - Prioridad: máquina de escribir (`cb_flg 0x200000`) > terminal (`0x400000`) > inventario > mapa (`0x2000`) > opciones (`0x8000`).
   - Inventario (system.c:608): `((!(st_flg & 0x4) && !(cb_flg & 0x4) && !(pad_on & 0x80) && (pad_ps & 0x4000)) || (cb_flg & 0x64010)) && !(cb_flg & 0x2000000) && PauseBtn == 0`. `0x64010` son peticiones automáticas: 0x10 objeto cogido, 0x4000 coger sin preguntar, 0x20000 archivo, 0x40000 baúl. `cb_flg 0x2000000` bloquea el inventario; no lo pone ningún código C (deducido: lo pone un guion).
   - Al abrir: `AllStopEnemySe`, `st_flg |= 0x8`, `mn_mode0 = 5`, **`sp_flg = 0x30`** (solo el planificador de eventos y la lectura del mando) y fundido de 5 frames.
3. **Cargador, modo 5** (system.c:1962-2036):

| Paso | Qué hace |
| --- | --- |
| 0 | `sbs_sp = memp`; copia las texturas de la sala a memoria principal |
| 1 | Lee `ITEM.AFS[145]` (la interfaz) en `subtxp` |
| 2 | Al acabar el fundido: `ts_flg \|= 0x80` (**tarea Game suspendida**) y libera las texturas de la sala |
| 3 | `SbsTextureInit` |
| 4 | `ItemTaskCheck()` (sub1.c:3031-3075): `ts_flg &= ~0x200` (**activa Itemselect**, tarea 9), `pad_ps = 0`, `subscreenmode \|= 1`, `StopVibrationEx` |

4. **Cada frame** (system.c:913-944): `bhSysCallItemselect` → `ItemTaskCheck()` → `StatusMain()`.
5. **Cierre:** `SpriteH` pone `statusflg 0x80000` al acabar la animación (sub1.c:2823). `ItemTaskCheck` espera a que el cargador esté libre (`taskloop == 4 && mn_md0 == 0`, sub1.c:3103) y entonces: `bhStandPlayerMotion()` sobre `plp`, limpia `cb_flg 0x10/0x20000/0x1C0000`, `ts_flg |= 0x200`, `st_flg &= ~0x8`, `ts_flg &= ~0x80`, `memp = sbs_sp`, devuelve las texturas, `pad_ps = 0` y `sp_flg = -1` (sub1.c:3103-3226).
6. Si se elige MAPA, se abre el mapa (`mpmd = 4`), que vuelve al inventario (system.c:856-906).

### Qué sigue funcionando con el inventario abierto

| Sigue | Se para |
| --- | --- |
| Pad (tarea 6) | **Game (7) entera**: jugadores, enemigos, objetos, efectos, cámara y todo el dibujo del mundo. Las texturas de la sala se descargan |
| Event (8): `scd1`, el planificador (`sp_flg 0x10`) y los mensajes del inventario (`bhControlMessage(1)`, system.c:728-742) | La cuenta atrás (`sp_flg 0x200`) |
| Itemselect (9), Monitor (20) y el sonido | |
| El reloj de juego (ps2_NaSystem.c:104 solo mira `tk_flg`) | |

## La pantalla: `StatusMain` (sub1.c:3242)

- **Init** (`case 1`, sub1.c:3263-3406): `StatusInit` (sub1.c:1389), o `ItemBoxInit` para el baúl, o `FileFlagInit01` + `GetFile` para archivos; después `CenterPositionInit`, `CursorInit`, `bhCheckSubPack` (sub1.c:3398) y `BgColorInit`.
- **Bucle** (`case 2`): `CursorMove`, `MainCommand`, `ItemCommand` (→ `TrigerSet`, `WeaponSet`, `ItemUse` → `Use_00/01/02/04/05`, `ItemCombination` → `Combi_*`), `ItemView` (itemview.c), animaciones y ventanas.
- Otros modos: `case 8` `GetItem` (coger un objeto), `case 4` `ItemBox` (baúl), `case 0x80` `ControlFileView` (archivos).
- Siempre: `Model_Read_Start/Set` (pide el **modo 6** del cargador para el modelo 3D del objeto, itemview.c:284-315), `CameraSet`, `DrawSubItem` y `SpriteOnOff` (→ `ItemSort`, `ItemSet`, `SidePackSet`, `KazuSet`/`BulletSet`, `ArmsSet`, `CursorSet`).
- **Mando:** solo `sys->pad_*` (sub1.c, itemview.c:572-759, fileview.c:393-1005, message.c:233-349). No usa `Pad[]`, `pdGetPeripheral` ni `p1per`.
- **`ply.` fijo:** ninguno en sub1.c, itemview.c ni fileview.c.

### Estado por jugador que toca

| Estado | Dónde (L = lee, E = escribe) |
| --- | --- |
| `plp->hp` | `StatusInit` sub1.c:1462-1485 (L, electrocardiograma); `Use_00` sub1.c:6309-6399 (E, curar) |
| `plp->stflg` | `StatusInit` sub1.c:1487 (L, veneno `0x280000`); `Use_00` sub1.c:6373-6426 (E); `ItemTaskCheck` sub1.c:3189 (L `0x1000000`) |
| `plp->wpnr_no` | `WeaponSet` sub1.c:4466-4566 (L/E); `GetItem` sub1.c:4153; `ItemBoxChange` sub1.c:7931 (E 0); `ItemTaskCheck` sub1.c:3180 (L) |
| `plp->mode3` | sub1.c:4584, 4151, 7930 (E 0) |
| `plp` entero | `bhStandPlayerMotion` al cerrar (sub1.c:3105; player.c:1122-1167): `mode0 = 1`, animación de reposo, `bhCalcModel` |
| `sys->ply_id` | Base del bloque (sub1.c:1448, 1647); **retrato** (`sprset[10].anim = ply_id`, sub1.c:1531-1538); `GetItem` sub1.c:4051, 4056; `standard[ply_id]` en el baúl (sub1.c:7867, 7888, 7950); `bhCheckSubPack` |
| `sys->ply_hp/wno/stflg` | No los toca |
| `gm_flg 0x8000000` (mochila) | L en 8 funciones; E en sub1.c:3392-3399 |
| `gm_flg 0x10000000` | E en `WeaponSet` sub1.c:4555, 4570 (id 131); lo lee el disparo (player.c:4966) |
| `ev_flg` 74 | Modo del arma id 10 (sub1.c:4527-4534, 4909-4919, 6051); lo lee el disparo. Se guarda con la partida |
| Otros | `spray_ct` (ranking, sub1.c:6302-6305); `sb_id` + `cb_flg 0x400` (`ItemUse` sub1.c:2677-2679); `cb_flg 0x200` + `flr_idx` (activador de suelo, `Use_01/05` sub1.c:6454-6458, 6530-6534); `mn_mode0 = 3` (sub1.c:4585, 4157, 7933) |

### Equipar, usar, combinar y examinar

- **Id de objeto → arma** (`WeaponSet`, sub1.c:4456-4586):

  | Id | `wpnr_no` | Id | `wpnr_no` |
  | --- | --- | --- | --- |
  | 0 | 0 (nada) | 9 | 4 |
  | 1 | 20 | 10 | 5 |
  | 2 | 12 | 11 | 18 |
  | 3 | 13 | 32 | 6 |
  | 4 | 11 | 33 | 7 |
  | 5 | 3 (pistola; la que da `COOP_TEST`) | 34 | 8 |
  | 6 | 14-17 según la munición cargada (bits `0x1000`/`0x2000`/`0x4000` de la mitad alta) | 55 | 1 (mechero) |
  | 7 | 10, o 19 con el bit `0x2000` | 131 | 3 (pone además `gm_flg 0x10000000`) |
  | 8 | 2 (cuchillo) | 142 | 9 |

- **Equipar:** `WeaponSet(slot)` (sub1.c:4456-4586), desde "Equipar" (sub1.c:4799-4805), al combinar con el arma equipada (sub1.c:5594, 5624, 5673, 5703) y desde `GetItem` (sub1.c:4124).
  1. Traduce el id a `plp->wpnr_no` (sub1.c:4468-4560). Volver a equipar la misma arma la desequipa.
  2. `pip[0] = slot`, `plp->mode3 = 0`, **`mn_mode0 = 3`**.
  3. **Cargador, modo 3** (system.c:1573-1608): lee `SYSTEM.AFS[plp->wpnr_no + ply_id*30 + 20]` en `memp`, llama a `bhReadWeaponData` (modelos en `sys->wrmdlp`/`wlmdlp`, `bhSetWeapon` sobre `obwp[0/1]` con `lkwkp = plp`, animaciones en `plp->mnwP[100+]`) y pide el banco de sonido. Ver [combat.md](combat.md).
  4. Ocurre con el inventario abierto: el cierre espera a que el cargador termine. No toca `ply_wno`, que se guarda en `bhPushGameData` o en la máquina de escribir.
- **Usar** (`ItemUse`, sub1.c:2662-2737): pone `sb_id` y `cb_flg |= 0x400`.
  - Curación (`Use_00`, sub1.c:6283-6447): cambia **`plp->hp`** (+50, +100 o lleno; ×2 en `gm_mode` 2; tope 160 o 320) y quita el veneno de `plp->stflg`.
  - Objetos clave (`Use_01/05`): solo funcionan si `cb_flg 0x200` está activo y `rom->flrp[flr_idx]` acepta el id. Entonces cierran el inventario y el script de la sala reacciona.
  - Si el objeto se gasta, se vacía la casilla y se corrige `pip[0]` (sub1.c:2716-2733).
- **Combinar** (`Combi_*`): solo cambian entradas de `pip`. Si afectan al arma equipada devuelven `0x80`, lo que llama a `WeaponSet` y recarga el arma.
- **Examinar** (`ItemView`): modo 6 del cargador; modifica entradas de `pip` (itemview.c:720, 772-776, 832). No usa `plp`.

## `swork.pip`: el inventario activo

`swork.pip` es un global que apunta al bloque del personaje actual. Lo usan la pantalla de estado y también el juego: la munición (`bhCheckBullet`/`bhCountBullet`/`bhSearchBullet`, ver [combat.md](combat.md)) y la máquina de escribir (`ItemSearch`/`EraseItem`). Se fija en `bhInitEvent` (event.c:365, en cada carga de sala), en `StatusInit`/`ItemBoxInit` y en `ExtraGameItemInit` (`cng_pid`).

## Baúles

- Se abren examinando con el botón de acción: zona tipo 4 con `attr & 0x2`, que pone `cb_flg 0x40000` (hitchk.c:4891-4905), o la tapa `bhObjItmBox` (objitm.c:799-831). Las variantes A y B usan `cb_flg 0x80000` y `0x100000` (hitchk.c:4893-4899).
- La pantalla del baúl es `StatusMain` en modo 4, con `pip` = inventario del personaje y `bxp` = baúl.
- El baúl general es compartido. `ItemBoxChange` aplica `standard[ply_id]` (el mechero y otros objetos fijos no se guardan) y reglas propias de `ply_id == 0`.

## Máquina de escribir

1. Un script activa `cb_flg 0x200000` → `bhCheckSubTask` (system.c:576-590).
2. `TypewriterInit` copia `plp` a `ply_*[ply_id]` (bup_00.c:356-377).
3. `mcWriteStartSaveFile` gasta una cinta (id 31) buscándola con `ItemSearch` en `swork.pip` (ps2_McSaveFile.c:140-164) y copia `version..save_end`.

## Battle Game

`gm_mode >= 3`: stage 5, sala 50, `ply_id = 3` (system.c:505-513). El opcode `bhExGameItemInit` (0xCF) llama a `ExtraGameItemInit` (sub1.c:8553-8600), que rellena `itm[cng_pid*16]` con una de 5 filas: `cng_pid` 0-3, más la fila 4 si `costume`.

## Opcodes de script de inventario

0x11 (`bhPlItemCheck`, probable), 0x27, 0x31, 0x4D, 0x89 (`bhPlayerChangeSet`), 0xB2 (`bhItemGetGet`), 0xB7 (`bhItemPlToSBox`), 0xB8 (`bhItemSBoxToIBox`), 0xBF (`bhPlItemLostEx`), 0xC1, 0xC2 (`bhItemGetGetEx`), 0xC7, 0xC9, 0xCF (`bhExGameItemInit`). Números comprobados contra `bhScenarioJmpT` (event.c:59); el detalle de los que no tienen nombre aquí está sin documentar.
