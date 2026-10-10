# Especificación — Cooperativo, hito 2b: P2 dispara

- **Fecha:** 2026-10-10
- **Estado:** decisiones del usuario tomadas en conversación (munición compartida con P1; vibración en el mando 2). El usuario pidió seguir ("adelante! continua para que pueda disparar"), como en 2a: spec, plan e implementación seguidos y revisión al final.
- **Contexto:** [docs/coop/README.md](../../coop/README.md) (D1-D7), la spec del hito 2a ([2026-10-10-coop-hito2a-design.md](2026-10-10-coop-hito2a-design.md)) y [combat.md](../../architecture/combat.md) (máquina de estados del combate, munición, objetos de arma, luces, vibración y daño).

## 1. Objetivo

Que P2 pueda apuntar, cambiar de blanco y disparar con el mando 2, con la misma clase de arma que lleva P1 (las manos de P2 ya son un clon del arma de P1 desde 2a), dañando a los enemigos y sin alterar a P1.

## 2. Comportamiento

### 2.1 Lo que hace P2

1. **Controles:** además de moverse, apunta (`0x10`), apunta arriba y abajo (`0x20`/`0x40`), cambia de blanco (`0x80`) y dispara (`0x100`). Sigue sin acción (`0x200`), aceptar (`0x800`) ni menús. Máscara: `0x5FF`.
2. **Armas con mira** (`WpnTab[wpnr_no].flg & 0x20`): P2 no puede apuntar con ellas (se le quita `0x10`), porque la mira cambia la cámara y lo que se dibuja para todos.
3. **Munición compartida con P1:** P2 dispara y recarga con la munición del inventario de P1 (`swork.pip`, sin cambios). Cuando P2 vacía el arma, el aviso de "arma vacía" (`gm_flg 0x40000`) queda puesto también para P1.
4. **Animación del arma:** la corredera, el bombeo y la apertura que provoca un disparo de P2 se ven en el arma de P2, no en la de P1.
5. **Fogonazo:** la luz del disparo de P2 sale en su mano. Hay una sola luz de fogonazo (`rom->lgtp[0]`): si P1 y P2 disparan en el mismo frame, gana el último.
6. **Impactos:** si P1 y P2 dan al mismo enemigo en el mismo frame, cuentan los dos.
7. **Vibración:** lo que provoca P2 (disparar, saltar escalones, empujar) vibra en el mando 2, con las mismas reglas que el mando 1 (opción de vibración del menú, eventos y demo). Al parar las vibraciones (inventario, fundidos, salida) se paran las de los dos mandos. Con el juego en pausa o P2 desactivado, el mando 2 no se queda vibrando.
8. **Cambio de arma de P1 con P2 apuntando:** P2 sale del modo de combate y vuelve a la postura de reposo.

### 2.2 Limitaciones aceptadas

- Los enemigos heridos por P2 reaccionan bien al impacto (la dirección sale de la bala), pero después siguen persiguiendo y atacando a P1.
- Las explosiones de las armas de P2 pueden dañar a P1, y nunca a P2.
- P2 sigue sin recibir daño ni morir.
- El ciclo de blancos (`ene[].stflg 0x800`) es compartido: al cambiar de blanco, P1 y P2 pueden saltarse uno.
- Los flags de partida de algunas armas especiales (`ev_flg` 74/75: ráfaga y lanzacohetes) se escriben igual si dispara P2.
- `bhCPM2_act_wpn` lee `ply.` directamente (player.c:4820-4829); solo afecta con `ply_id == 2`, y P2 siempre es Claire.

## 3. Estructura

| Archivo | Cambio |
| --- | --- |
| `include/ps2/veronica/prog/coop.h` | `COOP_P2_PAD_MASK 0x5FF`; prototipos de vibración de P2 |
| `src/ps2/veronica/prog/coop.c` | Contexto de P2 ampliado (puerto de vibración, objeto de arma, `gm_flg 0x40000`, impactos, fogonazo), máscara de mira, salida del combate al cambiar de arma; `COOP_TEST` |
| `src/ps2/veronica/prog/ps2_sg_pdvib.c` | Ganchos en `pdVibMxIsReady`/`pdVibMxStart`/`pdVibMxStop` para el puerto lógico 8 y bloque `#ifdef COOP` con la tabla de vibración de P2 |
| `src/ps2/veronica/prog/ps2_sg_pad.c` | En el bloque COOP: `coopPadActuater2()` al final de `Coop_pad_read2` |

Ganchos nuevos en código original (todos dentro de `#ifdef COOP`): G9 en las tres funciones `pdVibMx*` de ps2_sg_pdvib.c.

## 4. Componentes

### 4.1 Mando (`coopSetPad2`)

Máscara `COOP_P2_PAD_MASK` = `0x5FF`. Si `WpnTab[ply2.wpnr_no].flg & 0x20`, se quita además `0x10`. En la salida temprana (demo, `!(sp_flg & 0x20)`), se llama a `coopVibStop2()`.

### 4.2 Contexto ampliado (`coopBegin`/`coopEnd`)

Se añade a lo que ya guardan y restauran:

| Estado | `coopBegin` | `coopEnd` |
| --- | --- | --- |
| `CurrentPortId` (padman.c) | Guarda y pone 1: las vibraciones pedidas en la ventana de P2 van al puerto lógico 8 | Restaura |
| `gm_flg 0x40000` | Se guarda con el resto de `gm_flg` | Tras restaurar `gm_flg`, se le aplica el bit `0x40000` que dejó P2 (la munición es compartida) |
| `ene[i].flg & 0x4` (impacto en este frame), `i < 128` | Guarda qué enemigos lo tienen y lo quita | Lo vuelve a poner (OR) en los que lo tenían |
| `rom->lgtp[0]` (luz del fogonazo) | Copia la estructura entera | Si cambió y tiene `lkflg == 1`, la coloca en la mano de P2 (`njCalcPoint(&ply2.mlwP->owP[lkono].mtx, &lx, &px)`) y la deja fija (`lkflg = 0`) |

**Objeto de arma (revisión final):** si `coop_wpn_ok[0]`, se intercambian `flg & 0xC80000`, `mode0` y `mlwP` entre `sys->obwp[0]` y `coop_wpn[0]` **solo alrededor de `bhControlPlayer()`** en `coopControlPlayer2`, no en toda la ventana: los objetos de P2 se actualizan después dentro de la ventana y necesitan su propio `mlwP` (con el intercambio, `bhCalcModel` escribía las matrices de P2 en los `owP` de P1 y `bhActionWeapon` consumía los flags de P1). Es solo de `obwp[0]`, el único que escribe el código del jugador (`sys->obwp->…`, player.c:5343-6083; playpch.c:654-656, 817-819). Si el clon de las manos falló (`coop_wpn_ok[0] == 0`), los disparos de P2 animan el arma de P1.

**Fogonazo simultáneo:** si P1 y P2 disparan la misma arma en el mismo frame, `lgtp[0]` queda con los mismos bytes y la luz se queda en la mano de P1.

### 4.3 Vibración del mando 2 (ps2_sg_pdvib.c y ps2_sg_pad.c)

- **Estado de P2** en el bloque COOP de ps2_sg_pdvib.c: `Pad_act2[20]` y `Ps2_pad_motor2[6]` (alineado a 16).
- **Funciones:**
  - `coopVibIsReady2()`: como `pdVibMxIsReady` con el puerto 1 y `Ps2_pad_motor2`.
  - `coopVibStart2(param)`: como `pdVibMxStart` sobre `Pad_act2`.
  - `coopVibStop2()`: vacía `Pad_act2`, pone el motor a cero y lo envía al puerto 1.
  - `coopPadActuater2()`: como `Ps2_pad_actuater` sobre `Pad_act2`/`Ps2_pad_motor2`, con `scePadSetActDirect(1, 0, …)`.
- **Ganchos (G9):**
  - `pdVibMxIsReady`: `if (port == 8) return coopVibIsReady2();`
  - `pdVibMxStart`: `if (port == 8) return coopVibStart2(param);`
  - `pdVibMxStop`: `if (port == 8) { coopVibStop2(); return 0; }`. En el camino normal, además, `coopVibStop2()` antes de `Ps2_pad_act_all_stop()`, para que el inventario y los fundidos paren también al mando 2.
- **Envío:** `coopPadActuater2()` al final de `Coop_pad_read2` (bloque COOP de ps2_sg_pad.c), como `Ps2_pad_actuater` en `Ps2_pad_read`. La alineación del actuador del puerto 1 ya se hace (ps2_sg_pad.c, estado 4 de `Coop_pad_read2`).

### 4.4 Salida del combate al cambiar de arma (`coopCloneWeapon`)

Si `ply2.mode1 == 1` cuando se aplica una nueva arma, P2 vuelve a reposo: primero `coopLeaveCombatP2` (lo que hace `bhCPM2_act_wre` al bajar el arma: `stflg &= ~0x10400`, `flg &= ~0x10000`, `at_flg = 0`, `mtn_add = 0`; sin esto `stflg 0x10000` impide volver a apuntar, player.c:1797), y después `mode0..3 = 1, 0, 0, 0`, `mnwP = mnwPb` y la animación de reposo, con la misma secuencia que usa `coopPlaceNearP1` (`PlMtnAct[0][0][0]`, `bhSetMotion`). Sin recolocarlo.

`coopPlaceNearP1` (ocultar y recolocar en eventos) también llama a `coopLeaveCombatP2`.

`coopRoomStart` deja de borrar `flg & 0xC80000` y `mode0` de las manos de P2 (el motor no lo hace con las de P1 al cambiar de sala; borrarlos podía dejar la corredera desplazada).

### 4.5 Modo de prueba (`COOP_TEST`)

Solo si se compila con el define `COOP_TEST` (no está en `compile_config.json`): al terminar `coopLoadPlayer2`, si el inventario de P1 (`sys->itm[ply_id*16 + 2..9]`) no tiene la pistola (id 5), se pone en la primera casilla vacía con 15 balas (`0x0005000F`). Sirve para probar el disparo con la partida guardada del usuario, que solo tiene el mechero. El build normal no lo incluye.

## 5. Errores y robustez

| Situación | Resultado |
| --- | --- |
| P1 lleva el mechero o nada | P2 no tiene arma (`wpnr_no` 0): no puede apuntar, como hasta ahora |
| Arma con mira | P2 no puede apuntar con ella |
| P2 oculto o congelado | No se actualiza, no dispara; la vibración pendiente del mando 2 se agota sola en menos de 10 frames |
| Pausa, demo o entrada de P2 desactivada | `coopVibStop2()`: el mando 2 no se queda vibrando |
| Mando 2 desconectado | No hay entrada; el actuador sigue corriendo (se llama desde la lectura del puerto 2) |
| Opción de vibración desactivada | Se respeta: el filtro está antes de `pdVibMx*` (vibman.c) |

## 6. Verificación

1. Build sin avisos nuevos; `check_build.py` con `coopVibStart2`, `coopPadActuater2` y `coopControlPlayer2`.
2. Identidad sin `COOP` frente a `baseline_main.elf` (recompilando `player.o`, `effsub1b.o` o `ps2_SystemSaveScreen.o` si difieren).
3. Build de prueba con `COOP_TEST` en PCSX2, cargando la partida del usuario:
   - P1 equipa la pistola desde el inventario; las manos de P2 la llevan;
   - P2 apunta (`p2r1`, R1 → `0x10`) y dispara (`p2cross` o el botón de disparo de la configuración A): captura con P2 apuntando y con el fogonazo en su mano;
   - `ramread.py`: la munición de la pistola en el inventario de P1 baja con los disparos de P2;
   - P1 no se mueve ni dispara mientras P2 dispara, y su arma no se anima.
4. Vibración: no se puede comprobar con capturas. Revisión de código y checklist del usuario (con dos mandos reales).
5. Build final sin `COOP_TEST`.
6. Checklist manual: daño a enemigos, dos impactos en el mismo frame, vibración en el mando 2, pausa e inventario sin vibración pegada, armas de mira, escopeta y armas automáticas.

## 7. Fuera de alcance

- Que los enemigos ataquen o persigan a P2, y que P2 reciba daño.
- Arma distinta de la de P1 e inventario propio (hito 2c).
- Mira telescópica para P2.
