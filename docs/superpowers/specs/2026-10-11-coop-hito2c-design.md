# Especificación — Cooperativo, hito 2c: arma propia de P2

- **Fecha:** 2026-10-11
- **Estado:** pendiente de revisión del usuario. Decisiones del usuario: P2 empieza con el cuchillo; los disparos de P2 suenan con el banco de sonidos del arma de P1.
- **Contexto:** [docs/coop/README.md](../../coop/README.md), specs de 2a y 2b, [combat.md](../../architecture/combat.md) (animaciones y modelos de arma, sonido de armas, segundo jugador), [inventory.md](../../architecture/inventory.md) (formato de `itm`, id → arma), [world-systems.md](../../architecture/world-systems.md) (texturas, contador por índice global), [rooms-and-memory.md](../../architecture/rooms-and-memory.md) (memoria, cargador).

## 1. Objetivo

Que P2 lleve **su propia arma**, distinta de la de P1, con sus propios modelos de manos y arma, animaciones de arma, texturas y munición. Es la base del inventario propio (hito 2d), desde el que P2 podrá cambiar de arma.

En este hito el arma de P2 sale de su bloque de inventario (`sys->itm[256..]`), que se crea con el cuchillo. Todavía no hay pantalla para cambiarla: eso es el 2d.

## 2. Comportamiento

1. **Inventario de P2 en la partida:** bloque de 16 entradas en `sys->itm[256..271]`, con el mismo formato que el de un personaje (`[0]` = casilla equipada, `[1]` fija, `[2..11]` objetos). Metadatos en `itm[272..279]`. Ese rango está dentro de la partida guardada, de la instantánea de reintento y de las demos, y ningún código ni guion lo usa (comprobado en C y en los 205 scripts de sala). Se guarda solo, sin cambiar el formato.
2. **Partida nueva, o partida guardada sin bloque de P2:** el bloque se crea con el cuchillo equipado (`itm[256] = 2`, `itm[258] = 0x00080001`) y la firma en `itm[272]`.
3. **Arma de P2:** la del objeto equipado en su bloque, con la tabla de `WeaponSet` (id 8 → cuchillo, `wpnr_no` 2; ver inventory.md). Si es el mechero (id 55) o no hay nada, P2 va sin arma (`wpnr_no` 0, manos vacías).
4. **Manos y arma:** modelos propios cargados de `SYSTEM.AFS[20 + wpnr_no]` (P2 siempre es Claire). Dejan de ser un clon del arma de P1.
5. **Animaciones:** tabla propia de 512 `MN_WORK`: las del cuerpo (0-99) se copian de las de P1 (idénticas) y las del arma (100 en adelante) son las de su fichero.
6. **Munición propia:** P2 gasta y recarga de su bloque. El aviso de arma vacía (`gm_flg 0x40000`) y el crítico de la pistola especial (`gm_flg 0x10000000`) pasan a ser de cada jugador (en 2b el primero era compartido).
7. **Sonido:** los disparos de P2 suenan con el banco del arma de P1 (solo puede haber uno cargado). Con la misma arma suena bien; con otra, suena el disparo del arma de P1, o nada si P1 lleva cuchillo, mechero o nada.
8. **P1 no cambia:** su arma, animaciones, texturas, munición y sonido son los de siempre.

## 3. Estructura

| Archivo | Cambio |
| --- | --- |
| `coop.c` / `coop.h` | Buffers del arma de P2, `coopReadWeapon2Data`, sincronización de las animaciones del cuerpo, carga en G8, `coopMonitorWeapon2` (para el 2d), munición y bits de `gm_flg` por jugador, siembra del bloque |
| `dread.c` | G4: `coopCloneWeapon` (final de `bhReadWeaponData`) se sustituye por `coopSyncBodyMotions` al final de `bhReadPlayerData` |
| `system.c` | G11 nuevo: al principio de `case 3` del cargador |
| `effsub1.c` | G12 nuevo: en `bhEff007` (cargador de las armas 12/13) |

| Gancho | Dónde | Llamada |
| --- | --- | --- |
| G4 (cambia) | Final de `bhReadPlayerData` (dread.c) | `coopSyncBodyMotions()` |
| G8 (amplía) | Paso 10 del modo 1 | `coopLoadPlayer2()` también lee el arma de P2 |
| G11 | `bhSysCallMonitor`, `case 3` (system.c) | `if (sys->mn_md3 == COOP_MN_P2) { if (coopMonitorWeapon2() != 0) SET_SYS_MN_MD(0, 0, 0, 0); break; }` |
| G12 | `bhEff007` (effsub1.c:2356) | El objeto de arma sale de `coopWeaponObjOf(op)`: el de P2 si el efecto es de P2 |

## 4. Componentes

### 4.1 Memoria (`coopInitMemory`)

Por debajo de `mempb`, además de lo de 2a:

| Bloque | Tamaño | Uso |
| --- | --- | --- |
| `coop_mnw2` | 12.288 B (512 `MN_WORK`) | Tabla de animaciones de P2 |
| `coop_wmt2` | 49.152 B | Animaciones de arma de P2 (máximo real 47.684 B) |
| `coop_wmdl2` | 49.152 B | Modelos de mano derecha e izquierda con sus `owP` (máximo real ≈ 26,9 + 17,7 KB) |

Unos 108 KB: el margen medido en la peor sala pasa de ≈ 1,83 MB a ≈ 1,72 MB. La zona de manos clonadas de `lmmdlp` (2a) queda sin usar. `coop_wpn2_tex[2]` (texturas cargadas por P2) se pone a 0 aquí: tras `njReleaseTextureAll` los texlists viejos no valen.

### 4.2 Siembra y lectura del bloque (G8, `coopLoadPlayer2`, `case 0`)

Tras comprobar el demo:

- Si `itm[272] != COOP_MAGIC`: poner a 0 `itm[256..279]`; `itm[256] = 2`; `itm[258] = 0x00080001`; `itm[272] = COOP_MAGIC`; `itm[273]` = vida máxima (160, o 320 con `gm_mode == 2`) y `itm[274] = 0` (reservados para el hito 3).
- `coop_wpn2_no = coopItemToWpn(itm[256 + itm[256]])` (función pura con la tabla de `WeaponSet`, sub1.c:4468-4560; 55 → 0; casilla 0 → 0).

### 4.3 Carga del arma de P2

- **Carga completa (G8):** después del modelo de P2, leer `SYSTEM.AFS[20 + coop_wpn2_no]` en la misma zona temporal (`ALIGN_UP(memp, 64)`; el fichero mide como mucho 382.008 B y ya cabía el de cuerpo de 1,25 MB) y procesarlo con `coopReadWeapon2Data`.
- **Cambio de arma (para el 2d):** el inventario de P2 marcará el modo 3 con `mn_md3 = COOP_MN_P2`; G11 lo desvía a `coopMonitorWeapon2`, que espera `GetReadFileStatus() != 1`, pide el fichero y lo procesa. Sin `RequestArmsSoundBank`. El modo 3 siempre cabe en memoria: su pico (≤ 128 KB + 382 KB) es menor que el del inventario, que ya estaba abierto.

### 4.4 `coopReadWeapon2Data(datp)`

Copia reducida de `bhReadWeaponData` (dread.c:174-345) que no toca `plp`, `sys->obwp`, `wrmdlp`, `wlmdlp` ni `plwmtp`:

1. Comprobar tamaños: modelo + 256 + `obj_num·80` de las dos manos en `coop_wmdl2`; animaciones ≤ 48 KB.
2. Comprobar la memoria de texturas (suma de los bloques TIM2 ≤ `Ps2_free_texmemsize`), como `coopReadPlayer2Data`. Si no cabe, P2 queda sin arma (manos de `[20]` si ya estaban, o sin manos).
3. Liberar **solo** las texturas que cargó P2 antes (`coop_wpn2_tex[k]`) y compactar (`bhGarbageTexture`). Nunca un `texP` que no cargó P2.
4. Por mano: copiar el modelo, poner a cero `coop_wpn[k]` y configurarlo como `bhSetWeapon` sin efectos sobre `plp` (`flg 0x81` y `0x40000` si hay arma, `id 1210`, `type = wpnr_no`, `lkono` 9/13, `lkwkp = &ply2`, `mtx = mtxbuf`, `flr_no` de P2), `bhMlbBinRealize`, texturas (`flg 0x200`, `coop_wpn2_tex[k] = 1`) y `owP` a cero tras el modelo, alineado a 256.
5. Poner a cero `coop_mnw2[100..511]` y reubicar las animaciones de arma con `bhMnbBinRealize` (el bucle original no limpia los huecos vacíos).
6. `coop_wpn_ok[k]` según lo cargado. Si P2 está montado, aplicar el arma (`coopApplyWeapon`) y, si estaba en combate, `coopStandP2`.

### 4.5 Animaciones del cuerpo (G4, `coopSyncBodyMotions`)

Copia `((MN_WORK*)sys->plmthp)[0..99]` a `coop_mnw2[0..99]` al final de cada `bhReadPlayerData` (carga completa y cambio de personaje). Sus punteros van a `sys->plbmtp`, que esa función acaba de rellenar. Sin esta copia, tras un cambio de personaje P2 apuntaría a datos ya sustituidos. En `coopRoomStart`, `ply2.mnwP = ply2.mnwPb = coop_mnw2`.

### 4.6 Munición y bits por jugador (`coopBegin`/`coopEnd`)

- `swork.pip`: en `coopBegin`, `&sys->itm[256]`; en `coopEnd`, `&sys->itm[sys->ply_id * 16]`.
- `gm_flg 0x40000` y `0x10000000`: en `coopBegin` se ponen los de P2 (`coop_gm2`); en `coopEnd` se guardan en `coop_gm2` y se restaura el `gm_flg` de P1 entero (se quita la propagación de 2b).

### 4.7 `bhEff007` (G12)

El efecto del cargador de las armas 12/13 escribe `objP[2].evalflags` del objeto `sys->obwp[0]` desde el update de efectos, aunque el disparo sea de P2 (y con un modelo de menos de 3 huesos escribe fuera del array). El gancho elige `coop_wpn[0]` si el efecto está enganchado a `&ply2`, y comprueba `obj_num > 2` antes de escribir.

### 4.8 Lo que no cambia

- `coopSwapWeaponObj` alrededor de `bhControlPlayer()` (2b), ahora con el objeto de arma propio de P2.
- P2 oculto si `sys->ply_id != 0`, y desactivado en el demo.

## 5. Errores y robustez

| Situación | Resultado |
| --- | --- |
| El arma de P2 no cabe (tamaños o texturas) | P2 sin arma; mensaje en el registro |
| Lectura con error | P2 se queda con el arma anterior (o sin arma en la carga completa) |
| Mechero o nada en la casilla equipada | `wpnr_no` 0 (manos vacías) |
| Partida antigua sin bloque | Se siembra con el cuchillo |
| Cambio de personaje de P1 | Se vuelve a copiar la tabla del cuerpo; P2 sigue oculto mientras no sea Claire |

## 6. Verificación

1. Build, `check_build`, identidad sin `COOP`.
2. `COOP_TEST` de 2c: pone en el bloque de P2 una pistola con 15 balas, equipada (en vez de en el de P1).
3. PCSX2 con la partida del usuario (P1 con mechero):
   - P2 aparece con el cuchillo (build normal) o con la pistola (`COOP_TEST`), y P1 con el mechero;
   - P2 apunta y dispara su arma; `ramread`: la munición baja en `itm[256..]` y no en el bloque de P1;
   - P1 sigue con su mechero y su luz.
4. Medida de memoria con `ramread` (`mempb`, texturas libres) y `texmargin.py`.
5. Checklist manual: P1 y P2 con armas distintas a la vez; cambio de personaje y vuelta; reintento y cargar partida (el arma de P2 vuelve).

## 7. Fuera de alcance

- Cambiar el arma de P2 desde una pantalla (hito 2d).
- Un segundo banco de sonidos de arma.
- P2 en la parte de Chris.
