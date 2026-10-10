# Especificación — Cooperativo, hito 3: salud de P2 y enemigos que le atacan

- **Fecha:** 2026-10-11
- **Estado:** pendiente de revisión del usuario. Decisiones del usuario: cada enemigo persigue y ataca al jugador más cercano, sin cambiar de objetivo en mitad de un ataque o agarre; **game over si muere cualquiera de los dos**, con el reintento normal.
- **Depende de:** hito 2c (metadatos de P2 en `itm[272..]`). La curación de P2 llega con el hito 2d (su inventario).
- **Contexto:** [player.md](../../architecture/player.md#salud-daño-y-muerte), [combat.md](../../architecture/combat.md#daño-al-jugador), [enemies-and-npcs.md](../../architecture/enemies-and-npcs.md), [events-and-flags.md](../../architecture/events-and-flags.md) (parte F, game over).

## 1. Objetivo

Que el cooperativo tenga riesgo: P2 tiene su propia vida, los enemigos le persiguen, le golpean, le agarran y le muerden, puede envenenarse, curarse y morir, y su muerte termina la partida igual que la de P1.

## 2. Comportamiento

1. **Vida de P2:** propia, con el mismo máximo que P1 (160, o 320 en el modo fácil). Cojea y gira más despacio con poca vida, como P1. Se guarda con la partida y vuelve al reintentar (`itm[273]`, más el veneno en `itm[274]`).
2. **Enemigos:** cada enemigo elige al jugador vivo y visible más cercano. No cambia de objetivo mientras ataca, agarra o tiene al jugador en el suelo, ni más de una vez cada medio segundo. Los jefes, trampas, la grúa, el Spotter y la polilla (cuyos huevos usan modelos que Claire B no tiene) siguen yendo solo a por P1.
3. **Agarres de P2:** el mando 2 sirve para soltarse (machacar botones) y es el que vibra.
4. **Daño de efectos:** ácido, polvo de polilla, fuego, vómito de zombi y gas de Nosferatu alcanzan al jugador más cercano al efecto. Las explosiones dañan a los dos. El gas de sala afecta también a P2.
5. **Muerte:** si muere P2, salta el game over normal (como si muriera P1), y "Continue" lo maneja P1. Al reintentar, los dos vuelven al último punto de control con la vida que tenían.
6. **El veneno** baja la vida de P2 sin matarle, como a P1. Se cura con su inventario (2d).
7. **Empujes:** P2 ya se aparta solo de los enemigos (2a/2b); con este hito, cada enemigo empuja a su objetivo.

## 3. Ganchos

| Gancho | Dónde | Qué hace |
| --- | --- | --- |
| G17 | `bhControlEnemy`, alrededor de `bhJumpEnemy[ep->id](ep)` (eneset.c:377) | `coopEnemyBegin(ep)` / `coopEnemyEnd(ep)`: elige el objetivo y, si es P2, cambia `plp`, el mando y el puerto de vibración |
| G18 | `bhControlEffect`, alrededor de `bhJumpEffect*` (effect.c ≈ 817-835) | Para los efectos 256, 260, 265, 266, 269, 350 y 397: `plp` = jugador válido más cercano al efecto. Tras `bhEff127` (gas): comprobar la cabeza de P2 a mano |
| G19 | Principio de `bhCheckBombAtari` (weapon.c:1342) | `coopCheckBombP2(…)`: repite el daño de la explosión sobre P2 dentro de `coopBegin`/`coopEnd` |

## 4. Componentes

### 4.1 Vida de P2 (coop.c)

- Se quita la vida fija (`COOP_P2_HP`) de `coopRoomStart` y de cada frame.
- `coopRoomStart`: `ply2.hp = itm[273]`; veneno: `ply2.stflg |= itm[274] & 0x280000`; recalcular `dmlvl` como `bhResetPlayer` (player.c:1006-1030).
- Al final de cada `coopControlPlayer2`: `itm[273] = ply2.hp`, `itm[274] = ply2.stflg & 0x280000`. Así la instantánea de reintento, la máquina de escribir y la tarjeta lo copian sin más ganchos.

### 4.2 Elección de objetivo (G17)

Tabla propia por enemigo (`coop_tgt[128]`, más el frame del último cambio), reiniciada en `coopRoomStart`.

1. **Partes enganchadas** (`ep->flg & 0x80`): si cuelgan de un jugador (`lkwkp == &ply` o `&ply2`), ese jugador; si cuelgan de otro enemigo, el objetivo de ese enemigo.
2. **P2 es candidato** solo si está cargado, visible (`!coop_hidden`, sin `stflg 0x1000000` ni `0x80000000`), vivo (`hp >= 0`, `mode0 != 3`, y no `mode0 6` con `flg & 2`) y no es el demo. Si no, el objetivo es P1.
3. **Bloqueo:** no se cambia si el enemigo no está en su estado normal (`mode0 != 1`) o si su objetivo actual está siendo golpeado o agarrado (`mode0` 2, 4, 5 o 6, o `flg & 4` y cerca).
4. **Elección:** distancia en XZ (prefiriendo el mismo piso). Se cambia solo si el otro está a menos del 75 % de la distancia y han pasado 30 frames desde el último cambio.
5. **Lista de enemigos con elección de objetivo:** zombis (1, 26), arañas (3, 23, 24), Cerberus (4), Hunter (5), murciélagos (7), Bandersnatch (9), 10, Albinoid (21, 22) y 30. El resto, siempre P1.
6. **Si el objetivo es P2:** `plp = &ply2`, mando intercambiado con el de P2 (como `coopBegin`) y `CurrentPortId = 1`. `coopEnemyEnd` lo deshace en orden inverso. Con esto, soltarse de un agarre lee el mando de P2 y `sys->pad_on &= ~0xF` se aplica a P2.

### 4.3 Daño desde efectos y explosiones (G18, G19)

- G18 no cambia `plp` para los demás efectos: varios lo leen para otras cosas (fogonazo del cohete, etc.).
- G19 no recorre `ene[]` dos veces: solo repite el bloque del jugador para P2.
- Gas de sala: no se vuelve a llamar al efecto (cada llamada sube el gas); se compara la cabeza de P2 con `sys->gas_py`.

### 4.4 Muerte de P2

- El game over ya lo lanza el update de P2 (`bhCPM0_die`, `bhCPM0_enedie`), porque `coopBegin`/`coopEnd` no protegen `ts_flg` ni `gov_md0`. No hay que interceptarlo.
- **No reanimar a P2 muerto:** `coopPlaceNearP1` y `coopStandP2` no tocan `mode0` si P2 está muerto (`hp < 0`, o `mode0` 3 o 6). Si un evento oculta a P2 con `hp < 0`, se quita `stflg 0x40000` y se fuerza `mode0 = 3`.
- **Que acabe de morir:** con P2 muriendo, `coopControlPlayer2` lo sigue actualizando aunque haya mensajes (`coopFreezeCondition`), para que su animación llegue a lanzar el game over.
- `bhCPM0_die` pone a cero `sys->fade_*`; `coopBegin`/`coopEnd` los guardan y restauran.

## 5. Errores y robustez

| Situación | Resultado |
| --- | --- |
| Un enemigo cambiaría de objetivo durante un agarre | Bloqueado (4.2.3); si no, movería al otro jugador |
| P2 oculto, en una puerta o muerto | Nunca es objetivo |
| Jefes y enemigos de guion | Siempre P1 |
| Polilla | Siempre P1 (sus huevos usan modelos que Claire B no tiene) |
| Voz de P2 al recibir daño | Suena en la posición de P1 y puede cortar la voz de P1 (aceptado) |
| P2 y P1 mueren a la vez | Un solo game over (el bit solo se apaga una vez) |

## 6. Verificación

1. Build, `check_build`, identidad sin `COOP`.
2. PCSX2 (necesita una sala con enemigos; el usuario tendrá que llevar el juego hasta una, o una partida guardada con zombis cerca):
   - un zombi va a por el jugador más cercano; P2 recibe golpes y mordiscos y se suelta con el mando 2;
   - P2 cojea con poca vida; `ramread`: `itm[273]` baja;
   - P2 muere → game over → "Continue" → los dos vuelven con su vida del punto de control;
   - las explosiones y el ácido dañan a P2.
3. Checklist manual: Hunter, arañas, Cerberus; veneno; P2 curándose desde su inventario; guardar y cargar con P2 herido.

## 7. Fuera de alcance

- Que P2 reaparezca en lugar de terminar la partida (decisión del usuario: game over).
- Voz de P2 en su propia posición.
- Jefes que eligen objetivo.
- Fuego amigo de las explosiones entre jugadores (se mantiene: dañan a los dos).
