# El jugador y `BH_PWORK`

Rutas relativas a `src/ps2/veronica/prog/`.

## `BH_PWORK`: la entidad universal

`BH_PWORK` (types.h:238, 0x580 bytes) es el struct de **todo** lo que se mueve: el jugador, los enemigos y los NPCs. Grupos de campos:

| Campos | Para qué |
| --- | --- |
| `flg`, `flg2`, `mdflg`, `stflg` | Flags de estado. Ver [events-and-flags.md](events-and-flags.md). `stflg & 0x40000000` marca "es el jugador". |
| `id`, `type` | Para enemigos, el índice en `bhJumpEnemy[]` y su variante. |
| `mode0`..`mode3` | Máquina de estados jerárquica. `mode0` elige la rutina de primer nivel, `mode1` la de segundo, etc. Se suele escribir de golpe con `*(int*)&p->mode0 = 0x0102` (mode0 = 2, mode1 = 1). |
| `px, py, pz` / `ax, ay, az` | Posición y ángulos. Los ángulos son enteros Ninja: 0x10000 = 360°. |
| `pxb..azb` | Posición y ángulos del frame anterior. |
| `gpx, gpy, gpz` | Posición en el suelo. |
| `flr_no` | Número de piso. La altura del suelo es `rom->grand[flr_no + 2]`. |
| `mdl[16]`, `mlwP`, `mdl_n`, `skp[]`, `mbp[]`, `txp[]` | Modelos (`ML_WORK`: `objP` = árbol de huesos, `owP` = matrices por hueso). |
| `mnwP`, `mnwPb`, `mtn_no`, `frm_no`, `hokan_*` | Animación actual (banco de animaciones, número, frame, interpolación). |
| `ar, aw, ah, ad`, `cpcl` | Tamaño de colisión y cápsulas por hueso. |
| `wpnr_no`, `wpnl_no` | Arma en mano derecha e izquierda. |
| `hp`, `dam[64]` | Vida y daño recibido. |
| `exp0`..`exp3` | Punteros a bloques de trabajo extra, distintos según el tipo de entidad. |

## El jugador: `ply` y `plp`

- La instancia es `BH_PWORK ply` (main.c:40) y el puntero global es `BH_PWORK* plp = &ply` (main.c:59).
- **El código usa el global `plp`.** `sys->plp` (types.h:817) se escribe una vez en `bhInitSystem` y nadie lo lee.
- `plp` se asigna en solo tres sitios, y siempre a `&ply`: main.c:59, `bhInitSystem` (system.c:144) y `bhInitRoomChangePlayer` (player.c:857).
- Hay unos 6.900 usos de `plp->` en 60 archivos: player.c 2.532, pl_evt.c 349, playpch.c 273, playpch2.c 106, unos 3.000 en los `enNN.c`, y el resto en cut.c, event.c, room.c, hitchk.c, weapon.c, sub1.c, objitm.c…
- Las macros `PEXP0_*` y `EXP1_*` ([macros.h](../../include/ps2/veronica/prog/macros.h)) también expanden a `plp->exp0` / `plp->exp1`.
- **Casi todas las funciones del jugador son `void f(void)` y trabajan sobre `plp`.** Por eso se puede ejecutar el código del jugador sobre otra instancia cambiando `plp` temporalmente.
- No hay estado estático por jugador en player.c. Solo hay variables de uso momentáneo (Motion.c `mka_ang`, playpch.c:237 y :1133, pwksub.c:3246, weapon.c:504). El estado del jugador vive en `BH_PWORK` + `exp0..exp3` + `sys->obwp[0..3]`, más el inventario (`swork.pip`) y algunos flags globales (ver "Estado global que toca el código del jugador").

### Bloques `exp` del jugador

| Bloque | Tipo y tamaño | De dónde sale | Contenido |
| --- | --- | --- | --- |
| `exp0` | `EXP_WORK`, 0x7C (player.h:43) | `sys->plexwp` | Contadores de daño, puntos de pie (`fpx`) y de cuerpo (`bpx`), posición de partida (`spx`), objetivo de cámara (`plx/ply/plz`) |
| `exp1` | 124 B, "head work" | `sys->plhdwp` | Giro de cabeza |
| `exp2` | `PP_WORK`, 1712 B | `PlyPchInit` (playpch.c:12), en cada `bhSetPlayer` | Apuntado del torso y brazos |
| `exp3` | 32 KB | `sys->pletcp` | Simulación del pelo de Claire |

## Ciclo de vida

### Partida nueva

1. `bhFirstGameStart` (system.c:416): rellena los huecos por personaje (`ply_hp/ply_wno/ply_stflg[4]`) y pone `stg_no = 0`, `rom_no = 0`, `ply_id = 0` (Claire). El modo Battle Game usa stage 5, sala 50, `ply_id = 3`.
2. Cargador `bhSysCallMonitor` (system.c:1305), modo 1:
   - paso 4: `bhInitPlayer` (player.c:635) reserva los buffers fijos y fija `sys->mempb` justo después (player.c:659);
   - pasos 6-7: lee `SYSTEM.AFS[ply_id + 10 + costume*4]` y llama a `bhReadPlayerData` (dread.c:13);
   - pasos 8-9: lee `SYSTEM.AFS[20 + ply_id*30 + wpnr_no]` y llama a `bhReadWeaponData` (dread.c:170).
3. Modo 4: carga la sala (ver [rooms-and-memory.md](rooms-and-memory.md)) y termina en `bhFinishRoom` (room.c:337). Esta función coloca al jugador en `rom->posp[sys->pos_no]`, o en `sys->ply_pos` si se viene de cargar partida o reintentar (`cb_flg & 0x800000`).
4. En una partida nueva (`ss_flg & 0x100`) pone `plp->mode0 = 0`. En el siguiente frame, `bhControlPlayer` ejecuta `bhSetPlayer` (player.c:689).

### `bhSetPlayer` (mode0 = 0)

Configura el jugador a partir de `PlyInfo[ply_id]`. Lo importante:

- Fija `exp0/1/3` y la matriz.
- Engancha el objeto del pelo o accesorio en `sys->obwp[2]`: Claire (`ply_id` 0) y `ply_id` 3 (player.c:756-785).
- Crea la sombra (`bhSetShadow`), calcula el piso, pone la animación de reposo y llama a `bhCalcModel` y `bhCheckCut`.
- `PlyPchInit` reserva `exp2`.
- **`bhPushGameData`** (room.c:1016) guarda la instantánea para reintentar con la posición actual del jugador.

### Cambio de sala

`bhStartDoorDemo` → el cargador vuelve a `sys->memp = sys->mempb` → lee la sala nueva → `bhFinishRoom` → `bhInitRoomChangePlayer` (player.c:855). La memoria del jugador **sobrevive** porque está por debajo de `mempb`.

## Máquina de estados

`bhControlPlayer` (player.c:1349) solo se ejecuta si `sys->sp_flg & 0x1` y `!(plp->stflg & 0x1000000)`. Primero guarda la posición anterior, aplica el daño continuo (veneno o sangrado: `stflg & 0x281000` quita 1 HP cada 30 frames) y pasa a muerte si `hp < 0`. Después despacha:

`bhCtrPly_mode0[plp->mode0]()` (player.c:1333):

| mode0 | Rutina | Qué es |
| --- | --- | --- |
| 0 | `bhSetPlayer` | Inicializar |
| 1 | `bhCPM0_action` (:1777) | Control normal: `bhControlPlayerPad` (:1820) y luego `mode1` |
| 2 | `bhCPM0_damage` | Recibir daño |
| 3 | `bhCPM0_die` (:6346) | Morir (lanza el game over) |
| 4 | `bhCPM0_nage` | Agarrado por un enemigo |
| 5 | `bhCPM0_enedam` | Daño causado por un enemigo (deducido del nombre) |
| 6 | `bhCPM0_enedie` | Muerte causada por un enemigo (deducido del nombre) |
| 7 | `bhCPM0_event` (pl_evt.c:62) | Controlado por una cinemática (`pl_smove00..08`) |
| 8 | `bhCPM0_nothing` | Nada |

Dentro de `bhCPM0_action`:

- `mode1 = 0` → `bhCPM1_act_bas` (:1957), movimiento. Sus `mode2` son `bhCPM2_act_*`: `std`/`sta` (de pie), `wlk` (andar), `run` (correr), `bak`/`bk2` (atrás y giro rápido), `kdu`/`kdd` (escaleras), `dnu`/`dnd` (escalones), `psh` (empujar), `hsu`/`hsd` (escalerilla, probable), `rpsh`…
- `mode1 = 1` → `bhCPM1_act_atk` (:4481), combate: `suw`, `wpn`, `wre`, `atk` (disparar), `rld` (recargar), `knf` (cuchillo), más las variantes `_pch` (playpch.c) y `scp` (mira, playpch2.c).

Las animaciones de movimiento salen de `PlMtnAct[2][3][7]` (player.c:223). Los índices 0-99 del banco son animaciones de cuerpo y los de 100 en adelante, de arma (`PlMtnWpn`).

## Modelo y animaciones del personaje

### Buffers fijos (`bhInitPlayer`, player.c:639-653)

| Puntero en `sys` | Tamaño | Uso |
| --- | --- | --- |
| `plmdlp` | 128 KB | Modelo. Los `O_WORK` (`owP`) de cada modelo van justo detrás del modelo, alineados a 256 (dread.c:161-170) |
| `lmmdlp`, `wrmdlp`, `wlmdlp` | 32 KB cada uno | `lmmdlp` no lo usa nadie después de reservarlo (está por debajo de `mempb`: sirve para datos propios del cooperativo) |
| `plmthp` | 12 KB | Tabla de 512 `MN_WORK` (cuerpo 0-99, arma 100+) |
| `plbmtp` | 384 KB | Animaciones de cuerpo |
| `plwmtp` | 64 KB | Animaciones de arma |
| `plzmtp` | 8 KB | Datos z de animación |
| `plexwp`, `plhdwp` | 124 B cada uno | `exp0`, `exp1` |
| `pletcp` | 32 KB | `exp3` (pelo) |

En total son unos 724 KB, reservados con `bhGetFreeMemory` (pwksub.c:16) antes de `mempb`. Las texturas van a un pool aparte de unos 10 MB.

### Ficheros de personaje (`SYSTEM.AFS`)

| Entrada | Personaje | Tamaño |
| --- | --- | --- |
| `[10]` | Claire (`ply_id` 0) | 1.294.440 B (modelo 54 KB, animaciones 251 KB, texturas 987 KB en el fichero) |
| `[11]` | `ply_id` 1, probablemente Chris | 1.058.632 B |
| `[12]` | `ply_id` 2, sin confirmar (¿Steve?) | 999.496 B |
| `[13]` | `ply_id` 3, probablemente Wesker (empieza el Battle Game) | 1.134.056 B |
| `[14]` | Claire, traje alternativo (`costume` 1, "Claire B") | 1.249.352 B |
| `[20 + ply_id*30 + wpnr_no]` | Arma (con las manos). `[20]` = sin arma: solo las dos manos, de un hueso cada una, sin animaciones | 260-335 KB |

`sys->costume` solo cambia el índice del fichero de personaje (system.c:1427, 1431, 1769, 1773). `bhSetPlayer`, `bhReadPlayerData` y `bhReadWeaponData` no dependen de él, y el fichero de arma es el mismo con los dos trajes.

#### Formato del fichero de personaje

Todo en `u32` little-endian. El cargador lee el fichero entero en `sys->memp` y `bhReadPlayerData` (dread.c:17-175) lo reparte:

1. **Modelos:** `u32` de tamaño y una secuencia de bloques `{u32 size; data}` que acaba en `0xFFFFFFFF` (`size == 0` se salta) (dread.c:34-81).
   - Si `data` empieza por `SKIN_MAGIC` (0x4E494B53, dread.h:6), es la tabla de skinning del **siguiente** modelo: `skp[mdl_n] = data + 4`.
   - Si no, es un MLB (binfunc.c:4-96): byte 3 = estado (0x80 = modelo Cnk), `u16` en +4 = tamaño de cabecera, `u16` en +6 = `obj_num`, `i32` en +8 = offset de la texlist, `i32` en +12 = offset de los objetos. Los `NJS_CNK_OBJECT` (0x34 B) llevan `child`/`sibling`/`model` como offsets, con -1 = NULL.
2. **Animaciones de cuerpo:** `u32` de tamaño y un MNB por número de animación, terminado en -1; `size == 0` deja la ranura vacía (dread.c:85-118; binfunc.c:198-247).
3. **Datos z:** `u32` de tamaño y entradas terminadas en -1. Los lee `bhGetTransZ` (player.c:7309-7337).
4. **Texturas:** un bloque por cada modelo con `texP`: `u32` de tamaño (el bit 31 pide alinear a 32) y bloques de 32 B `{code, size}` con `TIM2`/`PLI`, terminados en -1 (dread.c:130-159; ps2_texture.c:82-302).

Qué escribe `bhReadPlayerData`: copia los modelos a `sys->plmdlp`, las animaciones a `sys->plbmtp` y los datos z a `sys->plzmtp`; rellena `sys->plmthp[0..99]`, `sys->bmt_size` y `sys->hd_pos`; en `plp` pone `mlwP`, `mdl[]` (con `bhMlbBinRealize`), `mdl_n`, `skp[]`, `mbp[]`, `txp[]`, `mnwP`, `mnwPb` y los `owP`. Lee `plp->skp[mdl_n]` antes de decidir si llama a `npSkinConvert` (dread.c:62), así que depende de que `plp` esté a cero. `hd_pos`, `bmt_size` y `wmt_size` solo se escriben: nadie los lee.

#### Contenido de los ficheros de Claire (comprobado con la ISO)

| Modelo | `[10]` Claire | `[14]` Claire B |
| --- | --- | --- |
| 0, cuerpo | Cnk, 22 huesos, una sola malla con skin en el hueso 1, 5 texturas | Igual, con 6 texturas y otra geometría |
| 1, 2 | Solo texlist: variantes de la cara para el parpadeo (`PlyEyeTab`, player.c:210, 6996) | Igual |
| 3 | 22 huesos sin texturas: modelo de colisión (effsub2.c:2311) | Idéntico byte a byte |
| 4, coleta | 4 huesos en cadena, con skin, 1 textura | Geometría idéntica byte a byte; otra textura |
| 5 | 1 hueso; uso sin confirmar | No existe |
| 6, 7 | Huevo y larva de polilla de `en27` (en27.c:62-66, 202; `bhCheckMothEgg`, player.c:1050) | No existen |

- **Las animaciones de cuerpo de `[10]` y `[14]` son idénticas byte a byte** (251.472 B, 60 de 100 ranuras con datos), y también los datos z y la pose de reposo del esqueleto. Las animaciones de Claire sirven para Claire B.
- Los cinco personajes comparten la jerarquía de 22 huesos: 0 raíz → 1 cadera (lleva la malla) → 2 → 3 columna → 4 pecho → 5 cabeza (`lkono` del pelo); brazo derecho 6-9 (9 = mano, `lkono` del arma derecha); brazo izquierdo 10-13 (13 = mano izquierda); piernas 14-17 y 18-21 (`PlyLegRoute`, `PlyFlip`, player.c:218-227). Los nombres son deducidos de las posiciones. Chris tiene otro banco de animaciones.
- **El cuerpo no tiene manos:** la tabla de skin no tiene vértices en los huesos 0, 9 y 13. Las manos que se ven son los objetos de arma `sys->obwp[0/1]`, cargados del fichero de arma (ver [combat.md](combat.md)). Sus texturas reutilizan el índice 0x73 del cuerpo de Claire.
- Texturas en el pool (únicas, no lo que ocupan en el fichero): Claire 276.480 B, Claire B 258.816 B. Usan índices globales distintos (0x65-0x6A/0x73/0x1B8A frente a 0x96-0x9C), así que las dos pueden estar cargadas a la vez. Ver el sistema de texturas en [world-systems.md](world-systems.md).

#### Bits de `owP[i].flg` (`O_WORK` por hueso)

| Bit | Efecto |
| --- | --- |
| `0x1` | `bhCalcTree` usa la matriz guardada (MdlPut.c:296) |
| `0x2` | La animación no escribe el ángulo de ese hueso (Motion.c:168) |
| `0x4` | La animación ignora el hueso (Motion.c:154; face.c:21-60) |
| `0x8` | El ángulo se interpola en orden YZX en vez de ZYX (`SetMtnFastHokan`, Motion.c:552-568, 969-1010) |

`bhSetPlayer` pone `0x8` en los brazos (`owP[7]` y `[11]`, player.c:858-859) sin condiciones, probablemente para que las mezclas de animación del hombro no den vueltas raras. **No interviene en el dibujo**: el dibujo solo mira `objP->evalflags & 0x8` (`NJD_EVAL_HIDE`) y `& 0x80000000` (MdlPut.c:130, 146-155).

### Cambio de personaje (Claire ↔ Chris)

Siempre se reutiliza la misma `ply`; nunca hay dos instancias.

1. El opcode `bhPlayerChangeSet` (event.c:6264) pone `sys->cng_pid` y `cb_flg |= 0x80`.
2. En la siguiente carga de sala (system.c:1725-1810), el juego guarda `stflg/wno/hp` del personaje actual, libera sus texturas y vacía `obwp[0..3]`.
3. Pone `ply_id = cng_pid` y vuelve a leer modelo y arma en los mismos buffers.
4. Llama a `bhResetPlayer` y `bhStandPlayerMotion`.

Los arrays `[4]` de `SYS_WORK` (`ply_hp`, `ply_wno`, `ply_stflg`) y el inventario `sys->itm[ply_id*16]` son **por personaje**, no para jugadores simultáneos.

### Compartir datos entre dos instancias

Esto importa para el modo cooperativo.

- **Se puede compartir:** texturas (`texP`) y la tabla de animaciones `mnwP = sys->plmthp`. `en27.c:61-66` ya lo hace, y `Player_controll` (event.c:9271) cambia entre `plmthp` y `rom->rmthp`.
- **No se puede compartir:**
  - **El árbol de huesos (`NJS_CNK_OBJECT` de `objP`).** `bhSetMotion` escribe la pose en `objP[].pos/ang` (Motion.c:158-179), la interpolación la vuelve a leer de ahí (Motion.c:240-333), y player.c y playpch.c editan ángulos directamente.
  - **Las matrices `owP`**: cada instancia necesita las suyas.
  - **La reubicación del binario:** `bhMlbBinRealize` reubica punteros sobre los propios datos y no es idempotente (binfunc.c:43-63). No hay que llamarlo dos veces sobre el mismo binario.
- **Ejemplo de clonado existente:** `face_bh.c:75-77` copia `objP` y `owP` para tener una instancia propia.
- **Skinning:** se aplica y se revierte alrededor de cada dibujado (njplus.c:813-835 y 1840+), así que dibujar dos instancias una detrás de otra es seguro.

## Estado global que toca el código del jugador

Si se ejecuta el código del jugador con otra instancia en `plp`, esto se ve afectado:

- **Objetos globales:** `sys->obwp[0]` y `[1]` son las armas, enlazadas a `plp` en `bhSetWeapon` (weapon.c:128); `sys->obwp[2]` es el pelo o accesorio. La luz del mechero es `rom->lgtp[1]` (player.c:1459-1495) y la del fogonazo `rom->lgtp[0]`.
- **Pelo:** `bhObjClpn` (objitm.c:2122) toma su buffer de simulación de `sys->pletcp` si `op->lkwkp == plp` (objitm.c:2146-2150); si no, reserva unos 20 KB con `bhGetFreeMemory` por encima de `mempb`, que se pierden al cambiar de sala. Un objeto de pelo de otra instancia actualizado con `plp` apuntando a su dueño usaría el buffer del pelo de P1.
- **Combate e inventario:** la munición vive en el inventario, a través del global `swork.pip`; `gm_flg 0x40000` marca "arma vacía"; y `bhCPM2_act_wpn` lee `ply.` directamente (player.c:4820-4829). Ver [combat.md](combat.md) e [inventory.md](inventory.md).
- **Flags de `sys`:** `st_flg` (p. ej. `0x4`, player.c:1732), `gm_flg` (primera persona, mira), `cb_flg`.
- **Otros campos de `sys`:** `sys->pl_htp` (escaleras) y la cámara `cam.*` (player.c:1783-1786). `sys->hd_pos` no lo toca el update: solo lo escribe `bhReadPlayerData` (dread.c:168).
- `bhPushGameData`, llamado desde `bhSetPlayer`.
- El game over, que lanza `bhCPM0_die`.
- **Funciones que reciben un `BH_PWORK*` pero leen `plp` igualmente:**
  - `bhCheckFloorP` (hitchk.c:5245) lee `plp->exp0`/`flg` y escribe los activadores globales `sys->cb_flg`/`flr_idx`.
  - Las paredes que hacen daño en `bhCheckWall`/`bhCheckWallEx` dañan a `plp` (hitchk.c:259-287, 967-1001).
  - `bhCheckExmAtari` (hitchk.c:4588), el botón de acción, sondea desde `plp`.
  - `bhSearchPlayer` (pwksub.c:664) y `bhCheckPlayer` (hitchk.c:5712) solo tienen en cuenta a `plp`.

## Segunda instancia (build `COOP`)

`coop.c` mantiene un segundo jugador, `BH_PWORK ply2`.

- **Modelo:**
  - Comparte geometría, texturas, skinning (`skp`) y animaciones (`mnwP = ply.mnwPb`).
  - Tiene su propia copia de `objP` (con `child`/`sibling` reubicados) y de `owP` para cada `mdl[i]`, en un pool de 64 KB reservado en `bhInitPlayer` antes de `mempb`.
  - El clon se repite tras cada `bhReadPlayerData`.
- **Memoria propia:** `exp0` y `exp1` propios; `exp2` sale de `PlyPchInit` en cada sala; `exp3 = NULL` (solo lo usa el objeto del pelo).
- **Update:**
  - `bhControlPlayer()` se ejecuta con `plp = &ply2` dentro de `coopBegin()`/`coopEnd()`.
  - Antes y después se anulan `psh_ct` y el bit 0x80 de `stflg`. Si no, el empuje automático de cajas acabaría con el objeto caja poniendo `mode3 = 6` a P1.
- **Nunca se ejecuta `bhSetPlayer` sobre `ply2`:** llama a `bhPushGameData`, engancha el pelo y cambia la cámara.
- **Ocultar:** `stflg & 0x1000000` oculta a la vez el modelo, el update y la sombra (`bhEff001`).
