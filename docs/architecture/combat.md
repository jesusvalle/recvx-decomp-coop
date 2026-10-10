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
- Otros usos de `obwp[0/1]`: cargador de las armas 12/13 (effsub1.c:2356), tintado de Alexia (en12.c:1193-1194), mira (screen.c:312, 323), game over (gameover.c:304-305), escenas que cambian las manos (`bhPlyHandChange`, event.c:7378-7455) y vaciado al cambiar de personaje (system.c:1749-1752).
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

## Segundo jugador (build `COOP`, hito 2b)

P2 combate con el mismo código (`bhControlPlayer` con `plp = &ply2`). Además de lo que protegía el hito 1, `coopBegin`/`coopEnd` (coop.c) cuidan el estado global del combate:

| Estado | Qué se hace |
| --- | --- |
| `gm_flg 0x40000` (arma vacía) | La munición es compartida (`swork.pip` sigue siendo la de P1), así que el bit que deja P2 pasa a P1 |
| `sys->obwp[0]` (corredera, bombeo) | Se intercambian `flg & 0xC80000`, `mode0` y `mlwP` con el objeto de arma de P2 **solo alrededor de `bhControlPlayer()`**: los objetos de P2 se actualizan después en la misma ventana y necesitan su propio `mlwP` |
| `ene[i].flg & 0x4` (impacto del frame) | Se quita antes y se repone después, para que el disparo de P2 no ignore a un enemigo al que ya dio P1 en ese frame |
| `rom->lgtp[0]` (fogonazo) | Si cambia en la ventana de P2 con `lkflg == 1`, se fija en la mano de P2 (`lkflg = 0`) |
| Vibración | `CurrentPortId = 1`: va al mando 2 (ver [input.md](input.md)) |

Al sacar a P2 del combate a la fuerza (cambio de arma de P1, eventos) hay que hacer lo que hace `bhCPM2_act_wre`: `stflg &= ~0x10400` y `flg &= ~0x10000`. La entrada al combate exige `!(stflg & 0x10000)` (player.c:1797) y ese bit solo lo quitan `wre`, `cro` y el daño (`coopLeaveCombatP2`).

Limitaciones: la IA del enemigo herido sigue usando a P1, las explosiones de P2 pueden dañar a P1, y las armas con mira no se pueden usar con P2.
