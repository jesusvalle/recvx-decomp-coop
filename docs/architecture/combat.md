# Combate: apuntar, disparar, munición y daño

Rutas relativas a `src/ps2/veronica/prog/`. Los números de línea son del árbol de trabajo con los ganchos del cooperativo (octubre de 2026): en player.c están unas 4-8 líneas por debajo de los de documentos anteriores. Si no cuadran, busca por el nombre de la función. Lo deducido va marcado como "deducido" y lo no comprobado como "sin confirmar".

## Máquina de estados del combate

- **Entrada** (player.c:1797-1810): bit lógico `0x10` (apuntar) y `plp->flg & 0x20000` ("lleva arma"). Ese flag lo pone `bhSetWeapon` cuando `wpnr_no > 1`. Pasa a `mode1 = 1`, `mode2 = 64`.
- `bhCPM1_act_atk` (player.c:4489) despacha `mode2` (player.c:4535-4573):

| `mode2` | Rutina | Qué hace |
| --- | --- | --- |
| 64 | `suw` | Sacar el arma y fijar el blanco más cercano |
| 65 / 66 / 67 | `wpn` | Apuntar recto, arriba o abajo |
| 68 | `wre` | Bajar el arma (al soltar `0x10`, player.c:4844-4851) |
| 69 | `atk` | Disparo o cuchillazo |
| 70 | `rld` | Recargar (player.c:5961) |
| 71 | `scp` | Mira telescópica (playpch2.c) |
| 72 | `knf` | Rebote del cuchillo contra la pared |
| 164 / 169 / 197 | `_pch` | Variantes para dos armas a la vez (`WpnTab.flg & 0x800`): 64 → 164 y 69 → 169 (player.c:4522-4533) |

### Botones

| Acción | Condición | Dónde |
| --- | --- | --- |
| Apuntar arriba / abajo | `0x20` / `0x40`, salvo armas con `flg & 0x1` | player.c:4724-4755, 4855-4923; playpch.c:551-579 |
| Disparo semiautomático | `pad_on & 0x100` con `0x10`, arma lista (`at_flg & 0x10`) y gatillo soltado (`!(at_flg & 0x40)`; se limpia al soltar `0x100`) | player.c:4936, 4832-4835 |
| Disparo automático | Armas con `flg & 0x40`: rama `ct0` / `fend_ct` | player.c:5567-5957 |
| Cambiar de blanco | `pad_ps & 0x80` → `bhSearchNextEnemy`; con dos armas, `bhCPM2_SearchPch` / `SetLockOnDirection` | player.c:5180-5199, 5943-5956; playpch.c:1100 |
| Mira | `0x80` hace zoom; `0x60` sube o baja con `pad_dy` | playpch2.c:262-273 |
| Recargar | **No hay botón.** Se recarga al disparar sin balas si `bhSearchBullet()` encuentra munición y `gm_flg & 0x40000` está activo | player.c:4936-4947, 5456-5462; playpch2.c:85-96 |
| Giro de 180° | Fuera del combate: `pad_ps & 0x400` con atrás (`bhCPM2_act_bak`/`bk2`, player.c:2616, 2751). En combate solo se gira con `0x4`/`0x8` a velocidad `rtspd` | |

El código del jugador lee los bits `0x1`-`0x400` (y `0x10000` en pl_evt.c), nunca `0x800` ni los de menú. Ver también [input.md](input.md).

### Del gatillo al impacto

- `bhCPM2_act_atk` construye un `GA_WORK` y llama a `bhCheckGunAtari` (player.c:5348-5398, 5624-5690).
- La mira hace lo mismo desde el hueso de la cabeza, `owP[5]` (playpch2.c:166-209).
- Con dos armas, `CheckGunHit` (playpch.c:1130-1171) dispara desde los huesos 9 y 13; el segundo disparo sale con `gun_delay` (playpch.c:1108-1116).
- **La boca del cañón sale del hueso de la mano del propio jugador** (`owP[9]`/`[13]`), no de los objetos de arma: player.c:4949, 4989, 5005-5007, 5366-5372, 5642-5647; pwksub.c:2762, 3302.

## `WPN_TAB` (player.h:7-41)

Tabla `WpnTab[]` por número de arma (`wpnr_no`).

| Campos | Para qué |
| --- | --- |
| `flg` | Ver la tabla de bits |
| `fend_ct` | Frames por disparo en automático (player.c:5569) |
| `at_cct` | Frames antes del final en que se admite el siguiente disparo (player.c:5514) |
| `ef_yct` | Frame del casquillo; en el cuchillo, fin de la ventana de golpe |
| `act_ct0..2` | Frames de bombeo y recarga (`act_ct0 + ply_id`) |
| `r`, `l`, `rn`, `rmax` | Radio de la bala, alcance y dispersión con la distancia |
| `wp_fps1`, `wp_fps2`, `wp_cps` | Boca del cañón, segundo fogonazo y expulsión del casquillo, relativos al hueso de la mano |
| `ltp`, `lr`, `lg`, `lb`, `lnr`, `lfr` | Luz del fogonazo |
| `hrate` | Interpolación de la animación al disparar |
| `ef_scale`, `hiteff` | Efecto del impacto |
| `snd_wpno` | Banco de sonido `ARMS_xxx.SPQ` |
| `seno0`, `seno1` | Sonidos de disparo y de bombeo |
| `vib_tp` | Vibración (−1 = ninguna) |

Bits de `flg` (deducidos del uso):

| Bit | Significado | Bit | Significado |
| --- | --- | --- | --- |
| `0x1` | No apunta arriba ni abajo | `0x200` | Ráfaga (con `ev_flg` 74) |
| `0x2` | Animación de corredera en `obwp[0]` | `0x400` | Apertura o recámara |
| `0x4` | Luz del fogonazo | `0x800` | Dos armas a la vez |
| `0x8` | Colisión contra las cápsulas `cpcl` | `0x1000` | Bombeo |
| `0x10` | Recarga con casquillos sueltos | `0x2000000` | Retroceso |
| `0x20` | Mira telescópica | `0x10000000` | Disparo instantáneo (`bhCheckGunAtari`) |
| `0x40` | Automática | `0x20000000` | Proyectil (el daño lo hacen los efectos) |
| `0x80` | Expulsa cargador | `0x40000000` | Cuchillo, sin munición |
| `0x100` | No hace ruido (`stflg 0x100`) | `0x80000000` | Atraviesa a todos los enemigos de la línea |

Ejemplos (deducidos): 2 = cuchillo, 3/4 = pistolas, 7/8/9 = dos armas, 11 = escopeta, 13/18 = armas con mira, 14-17 = lanzagranadas, 20 = lanzacohetes. `wpnr_no == 1` es el mechero.

**El daño no está en `WpnTab`**: sale de `EneDamNear/Mid/Far[id de enemigo 0..30][wno 0..21]` (weapon.c:16-117), según la distancia (< 15, < 40 y el resto; weapon.c:757-774, 830-847), más el combo y el crítico de cada enemigo (`bhEne_CalcDamage`, zonzon1.c:725).

## Munición

- **No hay campo de munición en `BH_PWORK`.** La munición es la cantidad de la entrada del arma equipada en el inventario: `swork.pip[swork.pip[0]] & 0xFFFF`. Formato de las entradas en [inventory.md](inventory.md).
- `swork.pip` es un **global** que apunta a `&sys->itm[ply_id*16]`. Lo fijan `bhInitEvent` (event.c:365, en cada carga de sala), `StatusInit` (sub1.c:1448), `ItemBoxInit` (sub1.c:1647) y el cambio de personaje (sub1.c:8577).
- Funciones:
  - `bhCheckBullet` (weapon.c:421): ¿queda munición?
  - `bhCountBullet` (weapon.c:439): resta una bala y pone `gm_flg 0x40000` ("arma vacía") al llegar a 0. La llaman player.c:5136, 5395, 5666; playpch.c:661, 824; playpch2.c:208.
  - `bhSearchBullet` (sub1.c:8862): recarga combinando munición de otras casillas (`Combi_99`, sub1.c:7114) y ordena con `ItemSort`.
- Munición infinita: bit `0x08000000` de la entrada (weapon.c:430, 448), o `plp->at_flg & 0x40000000` (nadie lo activa).
- **Ninguna de estas funciones comprueba que el objeto equipado coincida con `wpnr_no`.** La correspondencia entre id de objeto y arma está en `WeaponSet` (sub1.c:4456-4586).

## Animaciones y modelos de arma

- Tabla de animaciones: `sys->plmthp`, 512 `MN_WORK` de 0x18 bytes (player.c:648; types.h:66-75). Entradas 0-99: cuerpo (`bhReadPlayerData`, dread.c:89-118). Entradas 100 en adelante, hasta un `-1`: arma.
- `bhReadWeaponData` (dread.c:178-345):
  - lee el fichero `SYSTEM.AFS[20 + ply_id*30 + wpnr_no]` ya cargado en `memp`;
  - copia el modelo de la mano derecha a `sys->wrmdlp` (→ `obwp[0]`) y el de la izquierda a `sys->wlmdlp` (→ `obwp[1]`), 32 KB cada uno, con sus `owP` detrás (dread.c:247, 308);
  - sube las texturas con `bhSetMemPvpTexture` y libera las anteriores si `!(ss_flg & 0x100)` (dread.c:201-206, 262-267);
  - copia las animaciones a `sys->plwmtp` (64 KB) y las reubica con `bhMnbBinRealize` en `&plp->mnwP[100]` (dread.c:317-344).
- Se llama en tres sitios del cargador: partida (system.c:1447-1460), cambio de arma desde el inventario (modo 3, system.c:1577-1592, que además pide el banco de sonido `RequestArmsSoundBank`) y cambio de personaje (system.c:1797-1810).
- El fichero de cuerpo trae 100 huecos de animación (0-99) y el de arma 20 (100-119), pero la tabla necesita las 512 entradas (`bhCPM0_nothing` usa la 218, player.c:6733). El bucle de animaciones de arma (dread.c:325-339) **no** pone a NULL los huecos vacíos, al contrario que el del cuerpo (dread.c:112): quedan entradas viejas. `wmt_size` y `bmt_size` solo se escriben. Otros usos de `sys->plmthp`: la polilla (en27.c:61) y opcodes de guion (event.c:9511, 9548, 9637, 10962, 11182).
- Ficheros de arma de Claire (`SYSTEM.AFS[20..40]`): de 267.536 a 382.008 B (las armas dobles 7/8/9 son las más grandes); modelo derecho ≤ 26.108 B, izquierdo ≤ 17.076 B, animaciones ≤ 47.684 B. `[20]` son solo las manos (1 hueso); el `[22]` (cuchillo) trae 2 huesos. Los ficheros 41-49 miden 11 B (vacíos).
- `bhReadWeaponData` desreferencia `op->mlwP` sin comprobar NULL y, salvo en la primera carga (`ss_flg 0x100`, puesto por `bhFirstGameStart` y quitado por `bhFinishRoom`), libera el `texP` del arma anterior.
- Índices que usa el combate: `PlMtnWpn = {100, 104, 109, 114, 101}` (player.c:240); 102/107/112 (fin de disparo); 103/108/113 (rebote del cuchillo); 116 (recarga); 117 y +13/+17 (dos armas).

## Objetos de arma (`sys->obwp[0/1]`)

- **Son solo visuales**: dibujan las manos y el arma y animan corredera, bombeo y apertura. El disparo no lee nada de ellos.
- `bhSetWeapon` (weapon.c:128-194) pone el objeto a cero y lo configura: `flg = 0x81` (activo y enlazado; `0x40000` si `wpn != 0`), `id = 1210`, `type = wpn`, `lkono` = 9 o 13 (hueso de la mano), **`lkwkp = plp`**, `mtx = mtxbuf`. Efectos sobre `plp`: `exp0->wpntp` (0 si `wpnr_no < 10`, si no 1; lo usa `PlMtnAct`), `flg 0x20000` si `wpnr_no > 1`, y quita el bit `0x2` de `owP[7..9]`/`[11..13]`.
- **Actualización:** `bhControlObjItm` (objitm.c:282-397), solo para índices `< rom->obj_n`:
  - copia la ocultación (`stflg 0x1000000`) del dueño;
  - calcula la posición desde `owP[lkono]` del dueño;
  - si `flg & 0xC80000`, llama a `bhActionWeapon` (lee `plp->mode0`/`mode1` y `sys->ply_id`; escribe `objP[2].pos[0]`/`ang[1]`);
  - `bhJumpObject2[1210 - 1200]` = `bhObjWpn` (objitm.c:137) pone los ángulos de enfundado según `plp->mode1`/`mode0`;
  - `bhCalcModel`: matriz = `owP[lkono]` del dueño × traslación × rotación (MdlPut.c:240-253).
- **Dibujo:** `bhDrawObjItm` → lista `ob_hlg` (con `pt_flg & 0x1`) → `bhDrawObject` (objitm.c:631-680).
- Flags de animación que escribe el disparo en `obwp[0]`: `0x80000` corredera, `0x400000` bombeo, `0x800000` apertura (player.c:5343-5345, 5428-5449, 5619-5621, 6004-6019, 6083; playpch.c:654-656, 817-819).
- Otros usos de `obwp[0/1]`: cargador de las armas 12/13 (`bhEff007`, effsub1.c:2356, desde el update de efectos: escribe `objP[2].evalflags` del arma de `obwp[0]` aunque el disparo sea de otro; con un modelo de menos de 3 huesos escribe fuera del array), tintado de Alexia (en12.c:1193-1194), mira (screen.c:312, 323), game over (gameover.c:304-305), escenas que cambian las manos (`bhPlyHandChange`, event.c:7378-7455) y vaciado al cambiar de personaje (system.c:1749-1752).
- Huecos de `obwp[]`: 0/1 armas, 2 pelo o accesorio, 3 solo se vacía, 4 en adelante objetos de la sala (los guiones los indexan con datos de la sala).

## Del impacto al daño

`bhCheckGunAtari` (weapon.c:473-999) **no lee `plp`**:

1. Traza la línea desde la boca del cañón y comprueba las paredes (`bhCheckL2Wall`, escribe `sys->apos`/`ahtp`). Una pared explosiva llama a `bhSetExplosion`.
2. Radio de la bala: `r + min(rn·dist, rmax)`, el doble si se apunta arriba o abajo.
3. Candidatos (weapon.c:615-619): `ene[]` con `flg 0x1` y `0x20` (se le puede disparar), sin `0x2` (muerto), no ocultos y **sin `flg 0x4`**.
4. Que esté delante, colisión de la cápsula contra `watr` (o `cpcl` si `flg & 0x8`) y sin pared en medio. Se elige el más cercano, o todos si el arma atraviesa.
5. Escribe en el enemigo: `flg |= 0x4`, `dpx/dpy/dpz` (punto del impacto), `dvx/dvy/dvz` (vector de la bala), `dax/day`, `wpnr_no`, `djnt_no` (hueso más cercano), `dam[kno] += EneDam*[id][wno]` y `comb_flg 0x10/0x20/0x40`.

**El enemigo procesa el impacto en su siguiente update.** `bhControlEnemy` va antes que los jugadores (game.c:52-58). Cada enemigo procesa y borra `flg 0x4` (por ejemplo en01.c:2469-2476; eneset.c:504-511) y `bhEne_InitDamage` vacía `dam[]` (eneset.c:377-379; zonzon1.c:806-825).

- **Un impacto por enemigo y frame:** `flg 0x4` excluye al enemigo de las siguientes balas del mismo frame (weapon.c:619; cuchillo :1097).
- La dirección de la reacción sale de los datos de la bala (`bhEne_DGDirCheck` usa `dvx`, zonzon1.c:515-518, 795-802), no de `plp`.
- **Después, la IA (girarse, perseguir, atacar) usa `plp`**: en01.c:8228, 8311, 8918-8986. El Hunter (en05.c:3032) decide si el impacto cuenta según `plp->flr_no` y `plp->at_flg & 8`. Es una muestra, no se han revisado los 31 `enNN.c`.

Otras comprobaciones de impacto:

- **Cuchillo:** el golpe no está en `bhCPM2_act_knf` (eso es solo el rebote) sino en `bhControlPlayer` (player.c:1698-1736), cuando `mode2 == 0x45`, `mode3 == 1` y el frame está entre `KnfAtrTab[ply_id]` y `ef_yct`. Llama a `bhCheckKnifeAtari` (weapon.c:1002-1145), que usa `EneDamNear[id][2]` sin caída por distancia y, al chocar con una pared, pone `plp->mode0..3 = 1,1,72,0`.
- **Proyectiles:** `bhCheckFlyAtari` (weapon.c:1162-1171), solo contra `ene[]`.
- **Explosiones:** `bhCheckBombAtari` (weapon.c:1342-1411), llamado desde el efecto de la explosión (effsub1.c:4213): daña a `ene[]` y a `plp`.

## Estado global que toca el combate

Además del `BH_PWORK` del tirador (`mode1..3`, `at_flg`, `wax/way/waz`, `ayp`, `src_no`, `ct0/ct1/ct3`, `flg`, `stflg`), `exp0` (`wpntp`, `yrct`, `rtspd`, `arp`, `scp_ct`), `exp1` y `exp2` (`PP_WORK`, ver abajo):

| Estado | Dónde |
| --- | --- |
| `st_flg 0x4` "jugador ocupado": bloquea inventario, mapa y opciones (system.c:608, 623, 640) | player.c:4491, 1846, 6226 |
| `gm_flg 0x40000` "arma vacía" (recarga y fuego automático) | player.c:4642, 4940-4955, 5668, 5821; playpch.c:510, 826, 1003; weapon.c:462 |
| Mira: `gm_flg 0x40/0x80/0x800/0x2000/0x80000`, `st_flg 0x800000`, `pt_flg 0x1` | player.c:4690-4693, 5213, 5966-5983, 6128-6131; playpch2.c:42-66 |
| `cam.pe_ax/pe_pers/ppers/ax/axp` | player.c:1791-1792, 4690-4691, 6128-6129; playpch2.c:62, 264-268 |
| `gm_flg 0x10000000` (crítico de la pistola especial, id 131; lo pone el inventario) | player.c:4966; sub1.c:4555 |
| `ef_flg 0x2` (lanzador lineal, arma 18) | playpch2.c:134; player.c:4925 |
| `ev_flg` 74 (ráfaga del arma 5) y 75 (al disparar el arma 20, o la 18 si `gm_mode == 3`); se guardan con la partida | player.c:4981, 5117, 5132; playpch2.c:161 |
| Luz del fogonazo `rom->lgtp[0]` con `lkflg = 1` si el tirador tiene `stflg & 0x40000000`. `bhControlLight` la coloca con `plp` **después** del update de los jugadores | pwksub.c:2685-2734; light.c:724-726 |
| Luz del mechero `rom->lgtp[1]` (`wpnr_no == 1`) | player.c:1467-1503, 1776-1779 |
| Vibración `StartVibrationEx(0, vib_tp)` si el tirador tiene `stflg & 0x40000000`. El primer argumento es un atributo, no el puerto: siempre vibra el puerto 0 | pwksub.c:2736-2739; playpch2.c:211-214; sdfunc.c:3382-3404; ps2_sg_pdvib.c:46-89 |
| Sonido: `CallPlayerWeaponSeEx(pos, se, slot)` suena en la posición dada, alternando los canales 8/9, con el **único** banco `ARMS_xxx.SPQ`, el del arma de `ply` | sdfunc.c:1546-1579, 752-756; system.c:1592, 2261 |
| Efectos (casquillos `sys->yk_ct`, fogonazo y humo `sys->ef`, cargador `sys->mg_ct`, chispas): plantillas y anillos compartidos que reciben la posición o el tirador | pwksub.c:2741-2980, 3242-3369; weapon.c:874-991, 1632-1703 |
| `ene[].stflg 0x800` ("ya apuntado", para ciclar blancos); `suw` lo borra en todos | player.c:4590-4595; pwksub.c:305, 601-640 |
| Munición: `swork.pip` | weapon.c:421-470 |

- Selección de blanco: `bhSearchNearEnemy(pp)`, `bhSearchNextEnemy(pp)`, `bhCalcLockEneYR(pp)` y `bhSearchEnemy(pwP)` reciben el tirador (pwksub.c:170-323, 506-661); `SetLockOnDirection` usa `plp` (playpch.c:318-477).
- Funciones que reciben parámetro pero usan `plp`: `bhSetWeapon`, `bhObjWpn`/`bhActionWeapon`, `bhCheckBullet`/`bhCountBullet`, `bhCheckKnifeAtari`, `bhCheckBombAtari`, `bhSearchNearEnemy` (marca el blanco si `pp == plp`), `PlyPchMain`/`bhArmIkMdk`.
- **Acceso fijo a `ply.`**: el único fuera de la inicialización es `bhCPM2_act_wpn` (player.c:4820-4829), que lee `ply.at_flg` y `*(int*)ply.exp2 & 0x4` (`PP_WORK.mode`). Ese bit solo se activa con `ply_id == 2` y `wpnr_no == 8` (playpch.c:1180-1192, 429-432).
- playpch2.c:49 escribe `((SYS_WORK*)plp)->itm[95]`: por el offset (0x270 + 95×4 = 0x3EC) es en realidad `plp->at_flg`.
- Variables estáticas del camino (`tmpS`/`tmpD` playpch.c:237, `gap` playpch.c:1133, `igct` weapon.c:504, `hr`/`hl` pwksub.c:3246): temporales, inofensivas si los tiradores se actualizan uno tras otro.

## `PP_WORK` (`exp2`)

- `PP_WORK` (playpch.h:17-35): `mode`, `count`, `gun_delay`, `obj_r`/`obj_l`, ángulos de los brazos, lista de blancos (`LckTbl`, `SchLst`, `sch_hed`) y `hed_rate`.
- Lo reserva `PlyPchInit` (playpch.c:12) en cada `bhSetPlayer`. `PlyPchMain` solo hace algo con armas `0x800` (playpch.c:29).
- La mayor parte del estado de apuntado **no** está en `exp2` sino en `BH_PWORK`, `exp0`, `exp1` (`EXP1_I(0)` `0x4`/`0x1E0`, player.c:1808-1809) y los globales de la tabla anterior.

## Fuego amigo

- Balas, cuchillo y proyectiles solo recorren `ene[]` (weapon.c:615-617, 1093-1095, 1162-1171). El jugador no está en `ene[]`, así que no puede recibir disparos de otro jugador.
- Las explosiones dañan a `plp` (el jugador que se esté actualizando cuando corre el efecto, normalmente `ply`).

## Segundo jugador (build `COOP`, hitos 2b y 2c)

P2 combate con el mismo código (`bhControlPlayer` con `plp = &ply2`). Además de lo que protegía el hito 1, `coopBegin`/`coopEnd` (coop.c) cuidan el estado global del combate:

| Estado | Qué se hace |
| --- | --- |
| `swork.pip` (munición) | Desde el hito 2c, `&sys->itm[256]` (el bloque de P2) durante la ventana de P2; se restaura el valor anterior al salir. En el 2b era compartida |
| `gm_flg 0x40000` (arma vacía) y `0x10000000` (crítico de la pistola especial) | Desde el hito 2c, cada jugador tiene los suyos: los de P2 viven en `coop_gm2` y se ponen en `gm_flg` solo dentro de la ventana. El crítico de P2 se calcula al cargar su arma (id 131 equipado) |
| `sys->obwp[0]` (corredera, bombeo) | Se intercambian `flg & 0xC80000`, `mode0` y `mlwP` con el objeto de arma de P2 **solo alrededor de `bhControlPlayer()`**: los objetos de P2 se actualizan después en la misma ventana y necesitan su propio `mlwP` |
| `ene[i].flg & 0x4` (impacto del frame) | Se quita antes y se repone después, para que el disparo de P2 no ignore a un enemigo al que ya dio P1 en ese frame |
| `rom->lgtp[0]` (fogonazo) | Si cambia en la ventana de P2 con `lkflg == 1`, se fija en la mano de P2 (`lkflg = 0`) |
| Vibración | `CurrentPortId = 1`: va al mando 2 (ver [input.md](input.md)) |

Al sacar a P2 del combate a la fuerza (cambio de arma de P1, eventos) hay que hacer lo que hace `bhCPM2_act_wre`: `stflg &= ~0x10400` y `flg &= ~0x10000`. La entrada al combate exige `!(stflg & 0x10000)` (player.c:1797) y ese bit solo lo quitan `wre`, `cro` y el daño (`coopLeaveCombatP2`).

### Arma propia de P2 (hito 2c)

- **Cargador propio** (`coopReadWeapon2Data`, copia reducida de `bhReadWeaponData`): lee `SYSTEM.AFS[20 + wpnr_no]` (P2 siempre es Claire) y escribe en buffers de P2, nunca en `plp`, `sys->obwp`, `wrmdlp`, `wlmdlp` ni `plwmtp`. Monta `coop_wpn[0/1]` como `bhSetWeapon` (`lkwkp = &ply2`). Antes de cargar libera solo las texturas que cargó P2; si las nuevas no caben en el pool, P2 queda sin arma (nunca llega al `exit(0)`).
- **Arma:** la del objeto equipado en `itm[256..]`, traducida como `WeaponSet` (`coopItemToWpn`). El mechero (id 55) deja a P2 sin arma (`wpnr_no` 0): su luz es de P1.
- **Animaciones:** tabla propia (`coop_mnw2`, 512 `MN_WORK`). Las del cuerpo (0-99) se copian de las de P1 al final de cada `bhReadPlayerData` (G4, `coopSyncBodyMotions`); las del arma (100 en adelante) son las del fichero de P2, con el resto a cero. Los ficheros `[20]` (sin arma) y `[21]` (mechero) no traen animaciones de arma: P1 conserva las de su arma anterior y P2 se queda con la parte del arma a cero (el cuerpo no usa animaciones ≥ 100: `PlMtnAct` va de 0 a 54).
- **Cuándo se carga:** en la carga completa (G8, tras el cuerpo de P2) y en el modo 3 del cargador cuando lo pide el inventario de P2 (`SET_SYS_MN_MODE(3, 0, 0, COOP_MN_P2)`; G11 lo desvía a `coopMonitorWeapon2`, sin banco de sonido).
- **Texturas:** cada arma ocupa unos 135-205 KB de pool (los TIM2 de las dos manos, 67-102 KB cada una); la comprobación previa suma los bloques enteros del fichero (131-197 KB por mano), así que pide el doble. Si P1 y P2 llevan la misma, el recuento de referencias de `njReleaseTexture` (`count`) evita que una liberación de P1 deje sin texturas a P2.
- **Cargador de las armas 12/13** (`bhEff007`): G12 oculta el cargador en el arma del tirador (`coop_wpn[0]` si el efecto cuelga de `&ply2`) y comprueba que el modelo tiene más de 2 huesos.
- **Sonido:** el banco es el del arma de P1 (D13): P2 suena con el disparo del arma de P1 o no suena.
- **Ráfaga del arma 5** (`ev_flg` 74): es global; la decide el inventario de quien la equipa.

Limitaciones: la IA del enemigo herido sigue usando a P1, las explosiones de P2 pueden dañar a P1, y las armas con mira no se pueden usar con P2.

## Daño al jugador

- Lo escribe siempre el atacante en su update, sobre `plp`: enemigos (golpes, agarres), efectos y explosiones. Ver [player.md](player.md#salud-daño-y-muerte).
- **Efectos que dañan a `plp`** (comprobado leyendo sus cuerpos): `bhEff_E03_Acid` (256, ácido de araña), `bhEff_E06_Rinpun` (260, polvo de polilla; envenena), `bhEff_E12_FrameLiquid` (265) y `bhEff_E12_FloorBlood2` (266) (Alexia), `bhEff_E14_Fire` (269), `bhEff_Sub350` (350, effsub4.c) y el gas de sala `bhEff127` (`plp->hp = -1` si la cabeza queda por debajo de `sys->gas_py`). El 397 (`bhEff_E15_Poison`, Nosferatu) daña a través de `PoisonAttack` (en15.c), que escribe `plp->hp`, `stflg 0x200000` y `mode0`.
- **Explosiones:** `bhCheckBombAtari` (weapon.c) daña a `plp` (radio `0.7·ar`, `dmax` cerca y `dmin` lejos, sin pared en medio) y a `ene[]`.
- **Build `COOP` (hito 3):** G18 pone `plp` = el jugador más cercano al efecto para esos ids, y repite a mano sobre P2 la comprobación del gas; G19 repite sobre P2 el bloque del jugador de `bhCheckBombAtari` (las explosiones dañan a los dos).

## Sonido de armas

- Todos los sonidos de arma van al banco 1: `CallPlayerWeaponSeEx` fuerza `(SeNo & 0xFFFF00FF) | 0x100` y alterna los canales 8/9 (sdfunc.c:1546-1579); no suena nada mientras se carga un banco de armas.
- Hay cinco bancos fijos, `SE_BANK = {0, 4, 5, 6, 7}` (ps2_sg_sd.c:1868-1872), todos ocupados: 0 común (`COMMON.MLT`), 1 armas (`ARMS_xxx.SPQ`, 20 bancos), 2-3 sala, 4 voz (`CORE_xxx`). El banco lo fija la cabecera del `.SPQ` (`SPQ_HEADER.BankNo`, sdfunc.c:654). `LoadSoundPackFile` lee el `.SPQ` en `memp` con `bhGetFreeMemory`/`bhReleaseFreeMemory` (sdfunc.c:618-697).
- Casi todas las armas disparan con el sonido 261 (lista 5); la 10 y la 19 con el 271. Comunes: 257/258 cargador, 263/264 corredera, 260 sin munición, 265 bombeo, 275/277 cuchillo, 276 (player.c:1733).
- **Solo puede haber un banco de armas cargado**: el del arma de `ply`. Un segundo jugador con otra arma suena con el banco de P1 (sonido de otra arma) o sin sonido si P1 no lleva un arma con banco (cuchillo, mechero, nada).
- **Un segundo banco completo no cabe**, pero sí uno reducido. El driver del IOP tiene 8 puertos fijos y la RAM de la SPU2 está repartida al peor caso. Quedan dos huecos que dejó la reserva de reverb, de unos 64 y 55 KB (ver [world-systems.md](world-systems.md#driver-de-sonido-iop-y-memoria-spu2)). Los sonidos que usa el jugador (listas 1, 2, 4-9, 15, 16, 20, 29 y 30) ocupan como mucho 51.264 bytes por arma (`ARMS_013`); el cuchillo, 5.600. Como las muestras se direccionan con desplazamientos relativos, los programas de esos sonidos se pueden añadir al HD de otro puerto apuntando al hueco, sin tocar el IRX.
- `WpnTab[].snd_wpno` da el número de `ARMS_xxx.SPQ` de cada arma (cuchillo 12, mechero 19).
- Los proyectiles y explosiones (effsub1.c: 259, 266-270, 279-282) piden sus sonidos desde el update de efectos, fuera del contexto del tirador.
