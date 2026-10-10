# Especificación — Cooperativo, hito 2a: P2 con el traje de Claire B, manos y coleta

- **Fecha:** 2026-10-10
- **Estado:** diseño aprobado en conversación sección por sección. El usuario pidió hacer seguidos spec, plan e implementación y revisarlo todo al final.
- **Contexto:** [docs/coop/README.md](../../coop/README.md) (decisiones D1-D4), la spec del hito 1 ([2026-10-09-coop-hito1-design.md](2026-10-09-coop-hito1-design.md)) y [docs/architecture/](../../architecture/README.md), sobre todo [player.md](../../architecture/player.md) (formato del fichero de personaje), [combat.md](../../architecture/combat.md) (objetos de arma), [world-systems.md](../../architecture/world-systems.md) (texturas, objetos, pelo) y [rooms-and-memory.md](../../architecture/rooms-and-memory.md) (memoria y cargador).

## 1. Objetivo

Que P2 se distinga de P1 a simple vista y se vea completo: lleva el traje alternativo de Claire ("Claire B", `SYSTEM.AFS[14]`), tiene manos y tiene su propia coleta, que se mueve con su propia simulación.

Es la primera parte del hito 2. Las siguientes son 2b (P2 dispara con el mismo tipo de arma que P1) y 2c (inventario propio de P2 con Start, incluido equipar un arma distinta).

## 2. Comportamiento

### 2.1 Lo que hace P2

1. **Traje:** P2 usa el traje que no lleva P1. Si P1 va con el normal (`sys->costume == 0`), P2 es Claire B (`SYSTEM.AFS[14]`). Si P1 va de Claire B, P2 es la Claire normal (`[10]`).
2. **Animaciones:** las mismas de P1 (`mnwP = ply.mnwPb`). Los ficheros `[10]` y `[14]` tienen el mismo esqueleto y animaciones idénticas byte a byte.
3. **Manos:** P2 lleva en las manos **la misma arma que P1**: un clon visual de los objetos de arma de P1 (`sys->obwp[0/1]`). Cuando P1 cambia de arma, las manos de P2 cambian sin que P2 se mueva. P2 camina con las animaciones de esa clase de arma (`wpntp`), pero sigue sin poder apuntar ni disparar (la máscara del mando sigue en `0x40F`).
4. **Mechero:** si P1 lleva el mechero (`wpnr_no == 1`), P2 lo lleva en la mano pero con `ply2.wpnr_no = 0`. Así P2 no enciende ni apaga la luz del mechero, que es de P1 (`rom->lgtp[1]`, player.c:1467-1503).
5. **Coleta:** P2 tiene su propio objeto de coleta, con el modelo 4 de su fichero y su propia simulación. La coleta de P1 no cambia.
6. **Presencia:** P2 se oculta (y se recoloca junto a P1, como en los eventos del hito 1) mientras P1 no sea Claire (`sys->ply_id != 0`). Por ejemplo, en la parte de Chris. Vuelve a aparecer cuando P1 vuelve a ser Claire.
7. **Sin cambios respecto al hito 1:** controles, eventos (ocultar y congelar), colisión, invulnerabilidad y desactivación en el demo de atracción.

### 2.2 Limitaciones aceptadas

- Con `sys->ply_id != 0`, P2 no existe en pantalla. La parte de Chris queda para un hito posterior.
- Las texturas de P2 (unos 253 KB) se quedan en el pool de texturas también mientras P2 está oculto.
- Las limitaciones del hito 1 siguen: la cámara sigue a P1, los enemigos ignoran a P2, las pisadas de P2 suenan en la posición de P1 y P2 no abre puertas ni coge objetos.

## 3. Estructura

### 3.1 Archivos

| Archivo | Cambio |
| --- | --- |
| `src/ps2/veronica/prog/coop.c` | Cargador de P2, lectura del modelo, clon del arma, coleta, montaje, actualización y dibujo de los objetos |
| `include/ps2/veronica/prog/coop.h` | Prototipos nuevos (`coopLoadPlayer2`, `coopCloneWeapon`); `coopCloneModel` desaparece |
| `src/ps2/veronica/prog/dread.c` | G4 cambia de sitio (ver §3.2) |
| `src/ps2/veronica/prog/system.c` | G8 nuevo |
| `docs/coop/README.md`, `docs/architecture/*.md`, `CLAUDE.md` | Estado, ganchos, checklist y hallazgos |
| Herramientas fuera del repo (`.superpowers/`) | `ramread.py` (lee la RAM de un savestate) y funciones nuevas del arnés de PCSX2 |

### 3.2 Ganchos en el código original

Todos dentro de `#ifdef COOP`.

| Gancho | Dónde | Llamada | Cambio |
| --- | --- | --- | --- |
| G1 | `pdGetPeripheral` | `coopGetPeripheral2` | Ninguno |
| G2 | `bhSysCallPad` | `coopSetPad2` | Ninguno |
| G3 | `bhInitPlayer` | `coopInitMemory` | Reserva y reparte más memoria (§4.1) |
| G4 | **Final de `bhReadWeaponData`** (dread.c) | `coopCloneWeapon` | Sustituye a `coopCloneModel` al final de `bhReadPlayerData`, que se elimina |
| G5 | `bhFinishRoom` | `coopRoomStart` | Monta a P2 con su propio modelo, sus manos y su coleta |
| G6 | `bhMainSequence` | `coopControlPlayer2` | Actualiza también los tres objetos |
| G7 | `bhAllDrawModel` | `coopDrawPlayer2` | Dibuja también los tres objetos |
| **G8** | **`bhSysCallMonitor`, modo 1, principio del paso 10** (system.c) | `if (coopLoadPlayer2() == 0) break;` | Nuevo |

Por qué G4 se mueve: con un modelo propio, P2 ya no necesita clonar el cuerpo de P1. Lo que sí debe seguir a P1 es el arma, y `bhReadWeaponData` es el único sitio por el que pasan los tres caminos que cargan un arma: partida (modo 1), cambio de arma (modo 3) y cambio de personaje.

## 4. Componentes

### 4.1 Memoria (`coopInitMemory`, G3)

Todo se reserva antes de que `bhInitPlayer` fije `sys->mempb`, así que sobrevive a los cambios de sala.

| Bloque | Tamaño | De dónde sale | Uso |
| --- | --- | --- | --- |
| `coop_exp0`, `coop_exp1` | 0x7C + 124 B | `bhGetFreeMemory` (como en el hito 1) | `exp0`/`exp1` de P2 |
| `coop_pool` | 64 KB (`COOP_MODEL_POOL_SIZE`) | `bhGetFreeMemory` (como en el hito 1) | Modelos de P2 copiados del fichero y sus `owP` |
| Coleta: `exp0` y `exp3` | 16 KB + 4 KB | **`sys->lmmdlp`** (32 KB que el juego reserva y no usa) | Buffers de simulación del pelo de P2 |
| Manos: árboles `objP` y `owP` clonados | El resto de `lmmdlp` (12 KB) | `sys->lmmdlp` | Huesos propios de los dos objetos de arma de P2 |

- `coopInitMemory` también reinicia el cargador de P2 (§4.2) y marca a P2 como no cargado.
- Si cualquier reserva falla, o si `sys->memp > sys->endp` tras reservar, P2 queda desactivado. `bhGetFreeMemory` no comprueba `endp`.

### 4.2 Cargador (`coopLoadPlayer2`, G8)

Una máquina de estados que se ejecuta en el paso 10 del modo 1, cuando P1 ya tiene cuerpo y arma y la sala aún no se ha leído (`sys->memp` está prácticamente en `mempb`). Devuelve 0 mientras está ocupada (el paso 10 hace `break` y se repite el frame siguiente) y 1 cuando termina, falle o no.

1. **Inicio:** si el pool no está, si es el demo (`ss_flg & 0xC00000`) o si P2 ya está cargado, devuelve 1. Si no, elige el fichero (`14` si `sys->costume == 0`, si no `10`) y pide su tamaño con `GetInsideFileSize`. Un tamaño 0 desactiva P2.
2. **Comprobar memoria:** el fichero se lee en `ALIGN_UP(sys->memp, 64)` y debe terminar por debajo de `sys->endp − 327680`, que es la zona de trabajo de los TIM2 (`Ps2_PXLCONV`, main.c:140). Si no cabe, P2 queda desactivado.
3. **Pedir la lectura:** con `GetReadFileStatus() == 0`, `RequestReadInsideFile`. Si devuelve -1 (canal ocupado), se reintenta el frame siguiente.
4. **Esperar:** mientras `GetReadFileStatus() == 1`, devuelve 0. Si devuelve -1 (error de lectura), P2 queda desactivado.
5. **Procesar:** llama a `coopReadPlayer2Data(buffer)` (§4.3) y devuelve 1.

El cargador **siempre** carga a P2 (salvo en el demo), aunque P1 no sea Claire. Así P2 está listo cuando un cambio de personaje devuelva a Claire.

### 4.3 Lectura del modelo (`coopReadPlayer2Data`)

Copia reducida de `bhReadPlayerData` (dread.c:17-175) que escribe solo en datos de P2:

1. **Modelos:** copia la sección de modelos del fichero (`u32 dt0` + datos) a `coop_pool` y la recorre como dread.c:34-81.
   - Bloques `SKIN_MAGIC` → `coop_skp[n]`.
   - Los demás → `bhMlbBinRealize` sobre la copia, `npSkinConvert` si hay skin, y `coop_mbp[n]`/`coop_txp[n]`.
   - El resultado queda en `coop_mdl[]` y `coop_mdl_n`. `coop_skp` se pone a cero antes, porque `bhReadPlayerData` depende de que el `BH_PWORK` esté a cero.
2. **Salta** la sección de animaciones (`u32` de tamaño + datos) y la de datos z (`u32` + datos).
3. **Texturas:**
   - Antes de cargar, suma los tamaños de los bloques de textura de los modelos con `texP`. Es una cota superior: el pool solo guarda una vez cada índice global.
   - Si `Ps2_free_texmemsize` es menor que esa suma, P2 queda desactivado. Así se evita el `exit(0)` de `njLoadTexture` (ps2_NaTextureFunction.c:321-324).
   - Si caben, `bhSetMemPvpTexture(texP, datp, 0)` por modelo, con el bit `0x200` en `mdl[i].flg`, como dread.c:130-159.
4. **`owP`:** se reservan dentro de `coop_pool`, a continuación del modelo y alineados a 256 como en dread.c:161-167, y se ponen a cero.
5. Si todo cabe en `coop_pool`, se marca a P2 como cargado (`coop_loaded = 1`).

No toca `sys->plmdlp`, `plbmtp`, `plmthp`, `plzmtp`, `bmt_size` ni `hd_pos`. No llama a `bhMlbBinRealize` dos veces sobre los mismos datos: cada carga parte de una copia nueva del fichero.

### 4.4 Clon del arma (`coopCloneWeapon`, G4)

Al final de `bhReadWeaponData`, después de que `sys->obwp[0/1]` estén listos:

1. Para cada uno de los dos objetos (`k` = 0, 1):
   - copia el `O_WRK` entero a `coop_wpn[k]`;
   - arregla los punteros internos: `mlwP = &mdl[0]` y `mtx = mtxbuf`;
   - clona el árbol `objP` y los `owP` de `mdl[0]` en la zona de manos de `lmmdlp`, reubicando `child`/`sibling` como `coopCloneModel` del hito 1;
   - pone `lkwkp = &ply2`.
   - La geometría y las texturas se comparten con P1.
2. Guarda el arma en variables de P2: `coop_wpnr_no` (= `ply.wpnr_no`, o 0 si es el mechero), `coop_wpntp` (0 si `coop_wpnr_no < 10`, si no 1) y si lleva arma (`coop_wpnr_no > 1`).
3. Si P2 está montado y es visible, le aplica el arma al momento (§4.5, "Aplicar el arma"). **No** lo recoloca ni lo reinicia.
4. Si el clon no cabe en la zona de manos, P2 se queda sin manos (los objetos se marcan como inactivos), pero sigue existiendo.

`bhReadWeaponData` escribe en `wrmdlp`/`wlmdlp` antes de que G4 vuelva a clonar, y todo ocurre en la misma llamada, así que no hay ningún frame en el que P2 dibuje un clon que apunte a datos ya sobrescritos.

### 4.5 Montaje por sala (`coopRoomStart`, G5)

Como en el hito 1, con estos cambios:

- Si P2 no está cargado (`coop_loaded == 0`), no se monta.
- `ply2.mdl[]`, `mdl_n` y `mlwP` salen de `coop_mdl[]`; `skp`/`mbp`/`txp`, de `coop_skp`/`coop_mbp`/`coop_txp`. Se vuelven a poner en cada sala porque `ply2` se pone a cero.
- `mnwP = mnwPb = ply.mnwPb`.
- Bit `0x8` en `owP[7]` y `owP[11]`, como `bhSetPlayer`.
- **Aplicar el arma:** `ply2.wpnr_no = coop_wpnr_no`; `*(int*)ply2.exp0 = coop_wpntp`; `flg |= 0x20000` si lleva arma (si no, se quita); y se quita el bit `0x2` de `owP[7..9]` y `owP[11..13]` con la misma lógica que `bhSetWeapon` (weapon.c:128-194).
- **Coleta** (`coop_hair`, un `O_WRK` estático), rellenada como `bhSetObject(lkmtab, 2, …)` (objitm.c:154-215) más player.c:766-778:
  - `flg` de `lkmtab[0]` (129), `id` 1211, `mdlver` 4;
  - `lkono = 5`, `lox/loy/loz = (0, 1.5869, 0.7747)`;
  - `skp[0] = coop_skp[4]`, `mdl[0] = coop_mdl[4]`, `mlwP = &mdl[0]`;
  - `lkwkp = &ply2`, `mtx = mtxbuf`, escalas a 1.
  - **Además:** `flg |= 0x100000` y `exp0`/`exp3` apuntando a sus buffers de `lmmdlp`. Si no, `bhObjClpn` ejecutado con `plp == &ply2` usaría el buffer del pelo de P1 (`sys->pletcp`, objitm.c:2146-2150).
  - `mode0 = 0` reinicia la simulación en cada sala.
- **Manos:** `coop_wpn[0/1]` se reenganchan a `&ply2` y se reinician `mode0` y los flags de animación del arma (`0x80000`, `0x400000`, `0x800000`).

### 4.6 Ocultar

`coopHideCondition` añade dos condiciones a las del hito 1: `coop_loaded == 0` y `sys->ply_id != 0`. El resto del comportamiento de ocultar (recolocar junto a P1 en cada frame, sin update ni dibujo) no cambia.

### 4.7 Actualización (`coopControlPlayer2`, G6)

Después de `bhControlPlayer()` y `coopSeparate()`, **antes** de `coopEnd()` (porque `bhObjWpn` lee `plp`), para la coleta y las dos manos, si están activas, se repite lo que `bhControlObjItm` hace con un objeto enganchado (objitm.c:306-397):

1. Heredar la ocultación de `ply2` (`stflg & 0x1000000`).
2. Guardar la posición y los ángulos anteriores.
3. `njCalcPoint(&ply2.mlwP->owP[lkono].mtx, &lox, &px)`.
4. Si `flg & 0xC80000`, `bhActionWeapon`.
5. Manos: `bhObjWpn`. Coleta: `bhObjClpn`.
6. `bhCalcModel`.

Como en `bhControlObjItm`, solo se actualizan si `sys->sp_flg & 0x4`. Con P2 oculto o congelado no se actualizan.

### 4.8 Dibujo (`coopDrawPlayer2`, G7)

Después del cuerpo de P2, con las mismas condiciones (`pt_flg & 0x1`, P2 visible): `bhDrawObject` de la coleta y de las dos manos activas. En ese punto de `bhAllDrawModel` ya están puestas la luz media y el ambiente `amb_chr`, que es lo mismo que usa `bhDrawObjItm` para los objetos del jugador (objitm.c:518-529).

## 5. Errores y robustez

| Situación | Resultado |
| --- | --- |
| Falta memoria al reservar (G3) | P2 desactivado toda la partida |
| El fichero de P2 tiene tamaño 0, no cabe en la zona temporal o da error de lectura | P2 desactivado hasta la siguiente carga completa |
| `RequestReadInsideFile` devuelve -1 (canal ocupado) | Reintento el frame siguiente |
| Los modelos no caben en `coop_pool` | P2 desactivado |
| No hay memoria de texturas suficiente (cota superior) | P2 desactivado, sin cargar ninguna textura |
| El clon del arma no cabe | P2 sin manos |
| P1 no es Claire | P2 oculto |
| Demo de atracción | No se carga P2 (y sigue desactivado, como en el hito 1) |

- **Las texturas no se liberan a mano.** Las liberaciones globales (salir al título, reintentar, máquina de escribir) siempre van seguidas del modo 1, que vuelve a cargar a P2.
- **Cambio de personaje:** libera las texturas de P1 (`plp->mdl[i].texP` con `plp == &ply`) y compacta el pool. Las de P2 tienen índices globales distintos y se reubican solas (`Ps2ReplaceTexAddr`).
- **Riesgo conocido: el pool de texturas en salas futuras.** La comprobación del §4.3 protege la carga, pero no las salas posteriores. Se mide en §6. Si el peor caso calculado queda con menos de 512 KB libres, se para la implementación y se consulta al usuario.

## 6. Verificación

1. **Build** sin errores ni avisos nuevos.
2. **`check_build.py`** con los símbolos `coopLoadPlayer2`, `coopCloneWeapon` y `coopDrawPlayer2`. `coopCloneModel` ya no debe aparecer.
3. **Identidad sin `COOP`:** `cmp_load.py` contra `baseline_main.elf`, recompilando los objetos no deterministas (`ps2_SystemSaveScreen.o`, `player.o`).
4. **Medida de memoria** (herramientas nuevas):
   - **Arnés:** `Set-TestConfig` pone además `SavestateCompressionType = 0` (savestates sin comprimir); una función `SaveState` pulsa la tecla de guardado de PCSX2 (F1). `Restore-Config` lo deshace.
   - **`ramread.py`:** abre el savestate más reciente, lee `eeMemory.bin` y, con las direcciones del `elf/main.elf.xMAP` y los offsets de types.h, imprime `sys->memp`, `mempb`, `endp`, `Ps2_free_texmemsize` y el estado de P2 (`coop_loaded`, `coop_mdl_n`).
   - **Estático:** con los tamaños de textura de las 205 salas (el cálculo de la investigación) y la base medida en juego, calcular el peor caso del pool de texturas y de la RAM durante la carga, con P2.
5. **PCSX2, partida nueva en la celda.** Esperado:
   - P2 con el traje de Claire B, distinto del de P1;
   - P2 con manos y el arma de P1;
   - la coleta de P2 se mueve al andar y la de P1 sigue igual;
   - P2 anda y corre como en el hito 1.
6. **PCSX2, cambio de arma** (si en la celda P1 puede cambiar de arma; si no, en la primera ocasión): las manos de P2 cambian sin que P2 se mueva.
7. **PCSX2, puertas y demo:** P2 reaparece con su traje tras una puerta, y el demo del título sigue sin P2.
8. **Checklist manual del usuario** en `docs/coop/README.md`: lo que necesita dos mandos o progreso en la historia (reintentar, cargar partida, cambio de personaje, salas pesadas).

## 7. Fuera de alcance

- Disparar y apuntar con P2 (hito 2b).
- Inventario propio y arma distinta de la de P1 (hito 2c).
- P2 durante la parte de Chris u otros personajes.
- Liberar las texturas de P2 mientras está oculto.
