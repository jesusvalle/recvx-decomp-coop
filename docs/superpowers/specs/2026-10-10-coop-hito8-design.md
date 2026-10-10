# Especificación — Cooperativo, hito 8: P2 en escaleras y eventos

- **Fecha:** 2026-10-10
- **Estado:** aprobado el diseño en conversación; el usuario pidió spec, plan e implementación seguidos.
- **Decisiones del usuario:** alcance solo escaleras + eventos (puertas, examinar, objetos clave y cajas siguen en el hito 4); cuando P2 dispara un evento que toma el control, **P1 y P2 intercambian sus posiciones**.
- **Depende de:** hito 1 (cambio de contexto), 2d (`coopActionP2`, `coopRequestP2`), 3 (`coopP2Dead`, `coopP2Held`).
- **Contexto:** [world-systems.md](../../architecture/world-systems.md), [events-and-flags.md](../../architecture/events-and-flags.md) (`cb_flg`, parte D), [inventory.md](../../architecture/inventory.md#recoger-un-objeto) (tabla de tipos de zona de `bhCheckExmAtari`).

## 1. Objetivo

Que P2 use las escaleras, escalerillas, escalones y salientes de la sala igual que P1, y que las zonas de suelo que P2 pisa disparen los eventos y cinemáticas de la sala.

## 2. Comportamiento

1. **Escaleras (zona tipo 1, `kaidan` y escalerilla `attr & 1`):** P2 sube y baja con su botón de acción, con las mismas animaciones que P1. Una escalera que usa uno no la puede usar el otro hasta que termine (el motor ya la marca con `attr 0x400000`).
2. **Escalones y salientes (zona tipo 2 y bajada de saliente sin zona):** P2 sube y baja igual que P1.
3. **Escaleras que cambian de sala:** fuera de alcance (son zonas de puerta, tipo 0).
4. **Si se interrumpe a P2 a mitad de escalera** (un evento lo oculta, una puerta, recolocarlo junto a P1), la escalera se libera y P2 vuelve a estar de pie junto a P1.
5. **Activadores de suelo:** si P2 pisa una zona de suelo y P1 no pisa ninguna en ese frame, el guion de la sala ve la de P2 (`cb_flg 0x200`/`0x8000000` + `flr_idx`). Si los dos pisan una, gana P1. Las zonas "de frente" para objetos clave (tipo 0 con `attr & 1`) no se pasan.
6. **Intercambio:** si el activador de P2 hace que el guion tome el control (empieza una cinemática, el jugador pasa a guion o el guion lo oculta), P1 y P2 se cambian de sitio antes de que el guion lo mueva: la cinemática se rueda donde estaba P2, con P1. Si el activador no toma el control, nadie se mueve. Si el guion lanza una tarea de evento nueva que toma el control unos frames después (máx. 30), el intercambio se hace en ese momento.
7. Mensajes y puertas abiertas por guion no provocan intercambio.

## 3. Ganchos

| Gancho | Dónde | Qué hace |
| --- | --- | --- |
| G33 | `bhCheckExmAtari` (hitchk.c), al principio del cuerpo `if ((exp->flg & 0x1))` del bucle de zonas | `if (coopExmSkip(exp) != 0) continue;`: con `plp == &ply2`, se salta toda zona que no sea de tipo 1 o 2 |
| G34 | `bhSysCallEvent` (system.c), alrededor de `bhControlEvent()` | `coopEventPre()` antes y `coopEventPost()` después: intercambio de posiciones |

El gancho G33 va detrás de `#ifdef COOP` y hitchk.c incluye `coop.h` solo con `COOP`. Sin `COOP`, hitchk.c y system.c salen idénticos.

## 4. Componentes (coop.c)

### 4.1 Botón de acción de P2

En `coopRequestP2`, con las condiciones actuales de la acción de P2:

1. `bhCheckExmAtari(&ply2)` (dentro del contexto de P2: `plp == &ply2`; G33 filtra las zonas).
2. Si P2 ha empezado una escalera o un saliente (`ply2.stflg & 0x30` recién puesto), no se hace nada más. Si no, `coopActionP2()` como hasta ahora.

`bhCheckExmAtari` borra `cb_flg 0x100` y escribe `etc_idx`, `pl_htp` y `st_flg 0x4`: `coopEnd` restaura los de P1.

### 4.2 Escalera propia de P2

- Global `coop_pl_htp2` (`ATR_WORK*`). `coopBegin`: `sys->pl_htp = coop_pl_htp2` (después de guardar el de P1). `coopEnd`: `coop_pl_htp2 = sys->pl_htp` antes de restaurar el de P1.
- `coopRoomStart`: `coop_pl_htp2 = NULL` (la zona era de la sala anterior; `ply2` se pone a cero, así que `kdnp` también).
- `coopLeaveStairsP2()`: si `ply2.kdnp != NULL`, `bhClrUseKaidanFlag(&ply2)` y `kdnp = NULL`; si `ply2.stflg & 0x30`, limpia el estado como al final de `act_kdu` (`stflg &= ~0x80010030`, `flg &= ~0x80D0400`, `flg |= 0x118`, `flg2 &= ~0x1`). No toca `pl_htp`: solo se lee en los modos de escalera, que lo vuelven a poner al empezar. Se llama al principio de `coopPlaceNearP1` (que ya recoloca y pone de pie a P2), salvo con P2 muerto o agarrado.
- `coopSeparate` no empuja a P2 si cualquiera de los dos está en una escalera o saliente (`stflg & 0x30`) o si están en pisos distintos.

### 4.3 Activador de P2

En `coopControlPlayer2`, después del `bhControlPlayer` de P2 y antes de `coopEnd`: `trg = sys->cb_flg & 0x8000200`, `idx = sys->flr_idx` (son solo de P2, porque `bhCheckFloorP` los borra antes de recalcularlos). Después de `coopEnd`:

- si `trg != 0`, P1 no tiene `cb_flg & 0x8000200`, y la zona `idx` no es de tipo 0 con `attr & 1` (`coopFlrPtr(idx)`, que mira `rom->flrp` o `sys->mflrp` como `bhCheckFloorP`): y `coopCanSwap()` (si no se pueden intercambiar, el activador espera: P2 sigue en la zona): `sys->cb_flg |= trg`, `sys->flr_idx = idx`, `coop_trg_p2 = 1`. Tras el guion, `coopEventPost` quita esos bits y repone el `flr_idx` de P1, para que el inventario de P1 (`Use_01/05`) no vea la zona de P2.
- Si P1 sí tiene activador, se cancela la ventana de toma retrasada (`coop_swap_wait = 0`).

### 4.4 Intercambio (G34)

`coopCanSwap()`: P2 cargado, sin demo, visible (`coop_hidden == 0`), no muerto (`coopP2Dead`) ni agarrado (`coopP2Held`); los dos en `mode0 == 1`, sin `stflg & 0x30` (escalera o saliente) ni `stflg & 0x1000000`; sin `cb_flg & 0x5` (cinemática, puerta).

`coopTakeover(cb, md, hide)`: el guion ha tomado el control si `cb_flg 0x4` ha subido, `ply.mode0` ha pasado a 7 o `ply.stflg 0x1000000` ha subido respecto al estado anterior a `bhControlEvent`.

`coopEventPre()`:

1. Guarda `cb_flg`, `ply.mode0`, `ply.stflg` y el `status` de las 16 `bhEtask`, y `coop_can = coopCanSwap()` (después del guion ya no vale: P1 estará en `mode0 == 7`).
2. Si `coop_trg_p2` y `coop_can`: guarda la posición completa de los dos (`coopPosSave`: posición, `gp*`, `*b`, ángulo, piso y `EXP_WORK`), `coopSwapPos()` y `coop_swapped = 1`.
3. Sin activador de P2, un examen, objeto o mensaje de P1 (`cb_flg & 0x38`) cierra la ventana de toma retrasada.

`coopEventPost()`:

1. Si `coop_swapped`: con toma de control, se queda; sin ella, se deshace con `coopPosRestore` (no con otro `coopSwapPos`, que reiniciaría `arn`/`arp` de P1). Si alguna `bhEtask` pasó de `status == 0` a distinto de 0, `coop_swap_wait = 30`; si no, la ventana abierta sigue contando (con P2 en la zona esto se repite cada frame).
2. Si no hubo intercambio en este frame y `coop_swap_wait > 0`: con toma de control, `coopSwapPos()` si `coop_can`, y `coop_swap_wait = 0`; si no, `coop_swap_wait--`.
3. `coop_trg_p2 = 0`.

`coopSwapPos()`: guarda posición, ángulo (`ay`/`ayb`) y piso de cada uno y se los pone al otro con `coopSetPos` (posición, `gp*`, `*b` y los campos de `exp0`), más `bhCalcModel` de cada uno. Las sombras siguen a `lkwkp`. En pantalla partida, cada cámara se recalcula sola en el siguiente frame.

### 4.5 Reinicios

`coopRoomStart`: `coop_pl_htp2 = NULL`, `coop_trg_p2 = 0`, `coop_swapped = 0`, `coop_swap_wait = 0`.

## 5. Riesgos aceptados

- En la toma retrasada, los primeros frames del guion corren con P1 en su sitio. Casi todos los opcodes usan posiciones absolutas (137 teletransportar, 140 ir a un punto, cámaras fijas).
- Un activador que solo lanza enemigos o efectos en una tarea nueva no intercambia (no toma el control); si la tarea toma el control dentro de los 30 frames, sí.
- Con una sola cámara, P2 puede subir a un piso que la cámara de P1 no enseña (como ya pasa al alejarse).
- Una zona de suelo con `attr & 1` (de frente) que también lance una cinemática no la dispara P2.

## 6. Pruebas (manuales, PCSX2)

1. P2 sube y baja una escalera y una escalerilla dentro de una sala; P1 sigue moviéndose libremente mientras.
2. P2 sube y baja un saliente/escalón.
3. P1 intenta usar la escalera ocupada por P2: no puede hasta que P2 termina; después sí.
4. Un evento (o P1 cruzando una puerta) corta a P2 en mitad de la escalera: después P1 puede usar esa escalera.
5. P2 pisa la zona de una cinemática con P1 lejos: la cinemática empieza con P1 donde estaba P2; al acabar, P2 reaparece junto a P1.
6. P2 pisa una zona sin cinemática: nadie se teletransporta.
7. P2 no abre puertas ni examina con su botón de acción; sigue recogiendo objetos y abriendo el baúl.
8. Sin `COOP`: ELF idéntico byte a byte a la línea base (segmentos `PT_LOAD`).
