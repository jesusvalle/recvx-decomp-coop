# Modo cooperativo: análisis y decisiones

Objetivo: añadir un segundo jugador controlado con el mando 2 en el mismo juego (pantalla compartida, en PCSX2 o en una PS2 real). Se avanza por hitos, empezando por el más sencillo.

La arquitectura del motor en la que se basa este documento está en [../architecture/](../architecture/README.md). Cómo probar (herramientas, navegación en PCSX2, partida de prueba, identidad sin `COOP`): [testing.md](testing.md).

## Estado

| Fecha | Estado |
| --- | --- |
| 2026-10-09 | Análisis de viabilidad terminado (este documento). |
| 2026-10-09 | Entorno verificado: el ejecutable compilado sin cambios (SDK 2.0, build por defecto) juega en PCSX2 con los vídeos funcionando. Siguiente paso: especificación del hito 1. |
| 2026-10-10 | **Hito 1 implementado** (rama `coop-hito1`, sin commits). Spec: [2026-10-09-coop-hito1-design.md](../superpowers/specs/2026-10-09-coop-hito1-design.md). Plan: [2026-10-09-coop-hito1.md](../superpowers/plans/2026-10-09-coop-hito1.md). Probado en PCSX2: P2 aparece junto a Claire en la celda y se mueve con el mando 2 (confirmado por el usuario). Bug corregido: P2 desaparecía cuando P1 entraba en la animación de espera. Sin `COOP`, el ELF es idéntico byte a byte a la línea base. |
| 2026-10-10 | **Revisión final del hito 1** (revisor nuevo): ningún fallo crítico y uno importante, corregido y probado en PCSX2: P2 aparecía en el demo de atracción del título y podía desincronizarlo. Los menores están en el registro de ejecución. Siguiente: diseño del hito 2 (Claire B con manos y coleta, inventario propio, disparo). |
| 2026-10-10 | **Hito 2a implementado** (sin commits): P2 lleva el traje de Claire B, las manos con el arma de P1 y su propia coleta. Spec: [2026-10-10-coop-hito2a-design.md](../superpowers/specs/2026-10-10-coop-hito2a-design.md). Plan: [2026-10-10-coop-hito2a.md](../superpowers/plans/2026-10-10-coop-hito2a.md). Probado en PCSX2 cargando una partida guardada. Medido con `ramread.py`: margen de RAM en la peor sala ≈ 1,8 MB y del pool de texturas ≈ 1,4 MB. |
| 2026-10-10 | **Hito 2b implementado** (sin commits): P2 apunta y dispara con la misma arma que P1, con munición compartida; sus vibraciones van al mando 2. Revisión final: dos fallos críticos corregidos (manos de P2 sin actualizar, bloqueo para volver a apuntar). El usuario confirma en PCSX2 que P2 dispara. |

## Hoja de ruta

Leyenda: `[x]` hecho; `[ ]` pendiente. "(sin probar)" = implementado pero aún no comprobado en PCSX2. El orden de los pendientes es una propuesta.

### ✅ Hito 1 — P2 aparece y anda

- [x] Leer el mando 2 (puerto 1) con su propia máquina de conexión
- [x] P2 aparece junto a Claire al recibir el control y se mueve con el mando 2 (andar, correr, girar)
- [x] P2 entra en la animación de espera sin desaparecer
- [x] Eventos: P2 se oculta y se recoloca junto a P1 en cinemáticas, puertas y guiones; se congela con mensajes
- [x] P2 sigue a P1 al cruzar puertas
- [x] P2 choca con las paredes y no se solapa con P1 (sin probar)
- [x] Demo de atracción sin P2
- [x] Sin `COOP`, el ejecutable es idéntico al original

### ✅ Hito 2a — Aspecto de P2

- [x] Traje propio: Claire B (o el normal si P1 lleva Claire B), cargado en la carga completa
- [x] Manos: clon del arma de P1, que cambia cuando P1 cambia de arma (cambio de arma sin probar)
- [x] Coleta propia con su simulación, sin tocar la de P1
- [x] P2 se oculta mientras P1 no sea Claire
- [x] Memoria medida: margen de RAM y del pool de texturas en las peores salas

### ✅ Hito 2b — P2 dispara

- [x] Apuntar (R1) y disparar (X) con la misma arma que P1
- [x] Munición compartida con P1 (sin probar la recarga)
- [x] Los disparos de P2 dañan a los enemigos (sin probar)
- [x] El fogonazo sale en la mano de P2 y el arma de P1 no se anima con sus disparos (sin probar)
- [x] Dos impactos en el mismo enemigo y frame cuentan los dos (sin probar)
- [x] Las vibraciones de P2 van al mando 2 (sin probar)
- [x] Armas con mira bloqueadas para P2
- [x] Si P1 cambia de arma con P2 apuntando, P2 vuelve a reposo y puede volver a apuntar (sin probar)

### Hito 2c — Inventario propio de P2

- [ ] Start en el mando 2 abre la pantalla de inventario de P2 (el juego se pausa para los dos)
- [ ] Inventario de P2 guardado en `sys->itm[256..]` (entra en la partida guardada sin cambiar el formato)
- [ ] Munición propia de P2
- [ ] Usar objetos curativos sobre P2
- [ ] Equipar un arma distinta de la de P1 (cargador de arma propio: modelo, animaciones y texturas)
- [ ] Sonidos del arma de P2 cuando no coincide con la de P1 (el banco de sonido de armas es único)
- [ ] Decidir cómo consigue objetos P2: recoger, baúl o pasárselos P1

### Hito 3 — Los enemigos atacan a P2

- [ ] Cada enemigo persigue y ataca al jugador más cercano (apuntar `plp` al objetivo antes de actualizarlo)
- [ ] Agarres y mordiscos sobre P2 (duran varios frames y escriben en el mando)
- [ ] P2 recibe daño y se le ve herido
- [ ] Muerte de P2 sin game over mientras P1 siga vivo; game over si mueren los dos
- [ ] Cómo vuelve P2 tras morir (reaparecer junto a P1, curarse, etc.)
- [ ] Explosiones: dañan también a P2, y las de P2 no a P1 (o fuego amigo opcional)

### Hito 4 — P2 interactúa con el mundo

- [ ] Botón de acción para P2: examinar y leer mensajes
- [ ] Recoger objetos con P2
- [ ] Abrir puertas con P2 (la puerta lleva a los dos)
- [ ] Empujar cajas con P2 sin afectar a P1
- [ ] Usar objetos clave y resolver puzles con P2

### Hito 5 — Cámara y sonido para dos

- [ ] Política de cámara cooperativa (seguir a P1, punto medio, o teletransportar a P2 si sale del plano)
- [ ] Pisadas y sonidos de P2 en su propia posición

### Hito 6 — Eventos y cinemáticas con dos jugadores

- [ ] Que P2 no salte junto a P1 en guiones cortos de examen (puzles)
- [ ] Cinemáticas que muestran o colocan a P2 donde tenga sentido
- [ ] Mechero y otros objetos de evento con P2

### Hito 7 — Otros personajes y modos

- [ ] P2 durante la parte de Chris (banco de animaciones y arma propios)
- [ ] P2 en el Battle Game
- [ ] Activar o desactivar el cooperativo desde el juego (sin recompilar)

## Decisiones tomadas

| # | Decisión | Por qué |
| --- | --- | --- |
| D1 | **En el hito 1, P2 es un clon de Claire.** Comparte texturas y animaciones con P1; tiene su propio árbol de huesos, sus matrices y sus bloques `exp`. | Es lo más barato: no hace falta un segundo cargador de personaje ni ~1.7 MB extra (buffers + texturas). Decidido por el usuario el 2026-10-09. |
| D2 | **Enfoque de "cambio de contexto":** P2 es un segundo `BH_PWORK` estático (`ply2`). En cada frame, después de `bhControlPlayer()`, se hace `plp = &ply2`, se intercambia el estado del mando y se vuelve a llamar a `bhControlPlayer()`. Después se restaura todo. | Todo player.c trabaja sobre el global `plp` y lee `sys->pad_*`. Así P2 tiene todo el repertorio de movimientos (y más adelante apuntar, disparar y recibir daño) sin reescribir ~7.300 líneas. Se descartó un controlador propio para P2 porque habría que tirarlo en cuanto P2 necesite combatir. |
| D3 | **Todo el código cooperativo va detrás de `#ifdef COOP`**, preferiblemente en archivos nuevos (por ejemplo `coop.c`/`coop.h`). | Compilando sin `COOP`, el código del juego sale igual que sin el mod. Eso permite comparar con objdiff y no ensucia el código decompilado. |
| D5 | **El hito 2 se divide en 2a (aspecto), 2b (disparo) y 2c (inventario).** | El disparo con el mismo tipo de arma que P1 es barato; equipar un arma distinta (lo que da sentido al inventario propio) exige un cargador de arma propio. Decidido por el usuario el 2026-10-10. |
| D6 | **P2 lleva el traje que no lleva P1** (Claire B si P1 va con el normal) **y en las manos la misma arma que P1** (clon visual de `sys->obwp[0/1]`). | Las animaciones de los dos trajes son idénticas, así que P2 solo necesita su modelo y sus texturas. Las manos van con el arma; clonar la de P1 es la base del disparo de 2b. Decidido por el usuario el 2026-10-10. |
| D7 | **P2 se oculta mientras P1 no sea Claire** (`sys->ply_id != 0`). | P2 usa las animaciones de P1; con Chris no le valdrían. La parte de Chris queda para más adelante. Decidido por el usuario el 2026-10-10. |
| D8 | **En 2b, P2 comparte la munición de P1** (`swork.pip` no cambia). | Es lo más equilibrado sin inventario propio; en 2c pasará a ser suya. Decidido por el usuario el 2026-10-10. |
| D9 | **Las vibraciones de P2 van al mando 2.** | Tabla de vibración propia de P2 enviada al puerto 1; antes iban todas al mando 1. Decidido por el usuario el 2026-10-10. |
| D4 | **No se cambia la estructura de `SYS_WORK` ni de `BH_PWORK`.** El estado nuevo va en variables globales nuevas. | El rango `version..save_end` de `SYS_WORK` es el formato de la partida guardada y del reintento, y `bhInitSystem` usa un tamaño escrito a mano. Ver [rooms-and-memory.md](../architecture/rooms-and-memory.md). |

## Hito 1: P2 aparece y anda

### Alcance

- P2 aparece al lado de Claire en cuanto ella toma el control. Es en la celda (stage 0, sala 0, `rm_0000.rdx`), tras la cinemática inicial y el vídeo.
  - El intro de la celda **esconde y congela a Claire** con `plp->stflg |= 0x1000000` (evento 29) y lo quita al entregar el control (evento 16). Ver [events-and-flags.md, parte E](../architecture/events-and-flags.md).
  - Por tanto, P2 aparece en el primer frame en que P1 es visible y controlable.
- P2 anda, corre y gira con el mando 2.
- Los botones de apuntar, disparar y acción de P2 están bloqueados (bits `0x10`, `0x20`, `0x40`, `0x80`, `0x100`, `0x200` y los de menú).
- Eventos, en dos niveles (definitivo en la spec, §4.4):
  - **Ocultar y recolocar junto a P1:** `ply.stflg & 0x1000000`, `sys->cb_flg & 0x5` (puerta o cine), `ply.mode0 == 7` (guion).
  - **Solo congelar:** `sys->st_flg & 0x200` (mensaje) o `!(sys->sp_flg & 0x1)`.
  - `ply.flg & 0x10000` **no** sirve como señal de guion: el juego lo pone también en la animación de espera, al empujar y al recibir daño.
- Al cambiar de sala, P2 reaparece junto a Claire.

### Implementación

- **Código:**
  - [coop.c](../../src/ps2/veronica/prog/coop.c) y [coop.h](../../include/ps2/veronica/prog/coop.h);
  - el bloque `#ifdef COOP` al final de [ps2_sg_pad.c](../../src/ps2/veronica/prog/ps2_sg_pad.c), con la lectura del puerto 2 (está ahí porque `Pad_set` es `static`).
- **Ganchos en el código original** (una línea cada uno, dentro de `#ifdef COOP`):

  | Gancho | Función | Llamada |
  | --- | --- | --- |
  | G1 | `pdGetPeripheral` | `coopGetPeripheral2` |
  | G2 | `bhSysCallPad` | `coopSetPad2` |
  | G3 | `bhInitPlayer` | `coopInitMemory` |
  | G4 | `bhReadWeaponData` (en el hito 1: `bhReadPlayerData`) | `coopCloneWeapon` (en el hito 1: `coopCloneModel`) |
  | G5 | `bhFinishRoom` | `coopRoomStart` |
  | G6 | `bhMainSequence` | `coopControlPlayer2` |
  | G7 | `bhAllDrawModel` | `coopDrawPlayer2` |
  | G8 (hito 2a) | `bhSysCallMonitor`, modo 1, paso 10 | `if (coopLoadPlayer2() == 0) break;` |
  | G9 (hito 2b) | `pdVibMxIsReady`, `pdVibMxStart`, `pdVibMxStop` (ps2_sg_pdvib.c) | Puerto lógico 8 → vibración del mando 2 (`coopVibIsReady2`, `coopVibStart2`, `coopVibStop2`) |

- **Activar o desactivar:** con `"COOP"` en `defines` de `compile_config.json`. Al quitarlo hay que borrar `build/src/` antes de compilar.

### Checklist de prueba manual (dos mandos)

| # | Prueba | Estado |
| --- | --- | --- |
| 1 | Partida nueva: la intro se ve y P2 aparece junto a Claire al recibir el control | ✅ (usuario) |
| 2 | P2 anda, corre y gira con el mando 2; cada mando mueve solo a su personaje | ✅ moverse (usuario); falta confirmar la independencia y el giro de 180° |
| 3 | P1 quieto más de 10 s (animación de espera): P2 sigue visible | ✅ (usuario, tras el arreglo; los dos entran en espera) |
| 4 | Los botones de apuntar, disparar, acción y menús de P2 no hacen nada; los menús de P1 funcionan igual | Pendiente |
| 5 | P2 choca con las paredes; P1 empuja a P2 y no se solapan | Pendiente |
| 6 | P1 examina algo con P2 lejos: P2 se queda quieto, no salta junto a P1 | Pendiente |
| 7 | Escena de Rodrigo (mechero en la zona de la celda): P2 desaparece y reaparece junto a Claire | Pendiente |
| 8 | Salir de la celda y cruzar varias puertas: P2 reaparece siempre junto a Claire | ✅ (usuario: "sigue a la Claire 1 por las salas") |
| 9 | Pausa, inventario y mapa con P2 presente | Pendiente |
| 10 | Desconectar y reconectar el mando 2 en mitad de la partida | Pendiente |
| 11 | Cargar una partida guardada y reintentar tras morir: P2 aparece junto a P1 | Pendiente |
| 12 | Esperar en el título al demo de atracción: P2 no aparece y el demo se reproduce igual que en el original | ✅ (usuario, tras el arreglo de la revisión final) |
| 13 | P2 toca una pared que hace daño: P2 hace la animación de daño sin perder vida; P1 no se ve afectado | Pendiente |

### Hallazgos de la prueba

- **P2 no tiene manos** (resuelto en el hito 2a). El modelo del cuerpo no tiene manos: su tabla de skin no tiene vértices en los huesos 9 y 13. Las manos que se ven en P1 son los objetos de arma `sys->obwp[0/1]`, cargados del fichero de arma (`[20]` = manos sin arma), y P2 no tiene objetos de arma. Se resolverá en el hito 2. (Una primera versión de este hallazgo atribuía el efecto al flag 0x8 de `owP[7]`/`owP[11]`; ese flag solo cambia el orden de interpolación de los ángulos del brazo: ver [player.md](../architecture/player.md).)
- **P2 no tiene coleta** (resuelto en el hito 2a), como estaba previsto: el pelo es el objeto `sys->obwp[2]`, enganchado solo a P1.
- **Demo de atracción (corregido en la revisión final).** El demo del título carga la partida por la ruta normal, así que P2 aparecía en él, quieto. Además, `bhControlPlayerHead` (player.c:6982) llama a `rand()` en cada frame de P2, y el demo depende de `srand(1)` (system.c:1819) para que los enemigos repitan la grabación. Ahora `coopDemo()` (`ss_flg & 0xC00000`) desactiva a P2 durante el demo.
- **Paredes que hacen daño.** Con el cambio de contexto, la pared daña a `plp` = P2: P2 hace la animación de daño con voz, pero no pierde vida (`hp` se fija cada frame). La trampa es de un solo uso (hitchk.c:1000): si la toca P2 primero, ya no daña a P1. Se acepta.

### Se acepta que en este hito

- la cámara siga solo a Claire;
- los enemigos ignoren y atraviesen a P2;
- las pisadas de P2 suenen donde está Claire;
- P2 no pueda abrir puertas ni coger objetos.

### Piezas de trabajo (análisis previo a la spec)

> Esta sección y la siguiente son el análisis anterior a la spec y se conservan como registro. No coinciden del todo con lo implementado: no se reserva `exp3`, no se usa `bhSetFloorNum`, el mando de P2 no se pone a cero en `bhInitRoomChangeSystem`, y `hd_pos` y `rom->lgtp[1]` no se guardan (el update no los toca para P2). Lo que vale es la spec y el código: la lista real de estado protegido es la de `coopBegin`/`coopEnd`.

| Pieza | Qué hacer | Dónde |
| --- | --- | --- |
| 1. Leer el mando 2 | Leer el puerto 1 con su propio estado de conexión (`Pad_status2`) y su propio periférico estático (`pdGetPeripheral2`), **sin** llamar a `Ps2_Read_Key`. Sacar pad.c:137-278 a una función reutilizable que escriba en un estado de mando de P2 (global nuevo). Aplicar la misma condición `sp_flg & 0x20`, la máscara de cinemática, y no hacer nada en modo demo. Poner a cero en `bhInitRoomChangeSystem`. | ps2_sg_pad.c, pad.c, system.c (`bhSysCallPad`) |
| 2. Instancia y memoria de P2 | `BH_PWORK ply2` estático. Buffers propios: `exp0` (EXP_WORK), `exp1`, `exp3`, una copia del árbol `NJS_CNK_OBJECT` de cada modelo y sus `O_WORK`. Todo reservado **antes** de `sys->mempb` o en arrays estáticos. Se comparten `texP` y `mnwP = sys->plmthp`. | player.c (`bhInitPlayer`), dread.c, nuevo `coop.c` |
| 3. Inicializar P2 | Una versión reducida de `bhSetPlayer` **sin** `bhPushGameData`, sin tocar `obwp[2]`, sin `bhCheckCut` y sin cambiar la cámara. Crea su sombra con `bhSetShadow`. | nuevo `coop.c` |
| 4. Actualizar P2 | En `bhMainSequence`, después de `bhControlPlayer()` (game.c:49): guardar el estado global → intercambiar `sys->pad_*` con el mando de P2 (on, oncpy, ps, rs, old, onb, psb, oldb, ax, ay, dx, dy, ar, al) → `plp = &ply2` → `bhControlPlayer()` → `plp = &ply` → deshacer el intercambio → restaurar el estado global. Con la misma condición `sp_flg & 0x1`. | game.c |
| 5. Dibujar P2 | `bhCheckClipModel(&ply2)` + `bhPutModel(&ply2)` justo después del dibujo de P1 (game.c:370), con la misma condición `pt_flg & 0x1`. | game.c |
| 6. Cambio de sala | Después de `bhFinishRoom` (system.c:1723), colocar a P2 junto a P1 (copiar lo que hace room.c:504-519: posición, `gp*`, campos `spx/plx/bpx` de EXP_WORK, `bhSetFloorNum`, altura del piso), recrear la sombra y llamar a `bhCalcModel`. P2 está congelado mientras `cb_flg & 0x1` (puerta en curso). | system.c o room.c |

### Estado global que hay que guardar y restaurar en el update de P2

Es lo que `bhControlPlayer` toca y que no pertenece al jugador:

- `sys->cb_flg` y `sys->flr_idx`. `bhCheckFloorP` borra y activa los activadores de suelo; si no se restauran, P2 borraría los de P1 o dispararía eventos.
- `sys->st_flg`, `sys->gm_flg`, `sys->pl_htp` y `sys->etc_idx` (`sys->hd_pos` no: solo lo escribe dread.c:168).
- Los campos de `cam` que escribe player.c:1783-1786, y la luz del mechero `rom->lgtp[1]`.
- `sys->pad_onb/psb/oldb`. Examinar guarda ahí el mando, y P2 no debe dejar el suyo.

### Riesgos conocidos

| Riesgo | Mitigación |
| --- | --- |
| `pdGetPeripheral(1)` devuelve los datos del puerto 0 (caché estática por frame). | Usar un periférico propio para el puerto 1. |
| `Pad_rdata2` está a cero → stick = −128 → `bhSetPad` lo interpreta como stick a tope (P2 andaría solo). | Leer de verdad el puerto 1; si no está conectado o no es analógico, centrar el stick (0x80). |
| `Ps2_Read_Key` sobrescribe `Pad[0..3]` y el soft reset. | No llamarlo para el puerto 1. |
| `bhSetPlayer` guardaría la posición de P2 como punto de reintento y le quitaría el pelo a Claire. | Init propio para P2 (pieza 3). |
| Compartir el árbol de huesos haría que P1 y P2 tuvieran la misma pose. | Clonar `objP` y `owP` por modelo; `face_bh.c:75-77` es el ejemplo. |
| `bhMlbBinRealize` no es idempotente. | No volver a reubicar el binario; copiar las estructuras ya reubicadas. |
| `ene[]` y todo lo que está por encima de `mempb` se borra al cambiar de sala. | `ply2` estático y buffers reservados antes de `mempb`. |
| `bhCheckExmAtari` (botón de acción) abriría puertas o cogería objetos para todos. | Bloquear el bit `0x200` de P2 en el hito 1. |
| Las paredes que hacen daño dañan a `plp`. | Con el cambio de contexto se resuelve solo: durante el update de P2, `plp` es P2. |
| Los agarres de enemigos modifican `plp` y `sys->pad_on` fuera de la ventana de intercambio. | En el hito 1 no aplica (los enemigos ignoran a P2). |
| `gm_flg & 0x80001` desactiva la conversión de stick a dirección para los dos mandos. | Aceptado. |
| La muerte de P2 lanzaría el game over. | En el hito 1, P2 no recibe daño. |

## Hito 2a: Claire B, manos y coleta

### Implementación

- **Carga** (G8, `coopLoadPlayer2`): en el paso 10 del modo 1 del cargador se lee `SYSTEM.AFS[14]` (o `[10]` si P1 va de Claire B) en la zona temporal `ALIGN_UP(memp, 64)`. `coopReadPlayer2Data`, copia reducida de `bhReadPlayerData`, vuelca modelos, skin, `owP` y texturas en memoria de P2 (pool de 64 KB) sin tocar nada de P1. Antes de cargar las texturas comprueba que caben, para no llegar al `exit(0)` del pool. No se carga en el demo.
- **Manos** (G4, `coopCloneWeapon`, al final de `bhReadWeaponData`): clona los dos objetos de arma de P1 con huesos propios en `sys->lmmdlp` (32 KB que el juego reserva y no usa), enganchados a `ply2`. Se repite en cada carga de arma (partida, cambio de arma, cambio de personaje). Con el mechero, P2 lo lleva en la mano pero con `wpnr_no = 0`, para no tocar la luz de P1.
- **Coleta** (`coopSetHair`): un `O_WRK` propio con el modelo 4 de Claire B y sus buffers de simulación en `lmmdlp`, con `flg 0x100000` puesto de antemano para que `bhObjClpn` no use el buffer del pelo de P1 (`sys->pletcp`).
- **Update y dibujo:** los tres objetos se actualizan dentro de `coopBegin`/`coopEnd` (lo que hace `bhControlObjItm` con un objeto enganchado) y se dibujan con `bhDrawObject` después del cuerpo de P2.
- **Ocultar:** además de las condiciones del hito 1, P2 se oculta con `sys->ply_id != 0`.
- **Medida en tiempo de ejecución:** el arnés de PCSX2 guarda savestates sin comprimir y `ramread.py` lee de `eeMemory.bin` `memp`, `mempb`, `endp`, `Ps2_free_texmemsize` y el estado de P2 (ver CLAUDE.md).

### Medidas (sala 0-1, partida cargada)

| Valor | Medido |
| --- | --- |
| `endp − mempb` | 11.094.656 B → margen durante la carga de la peor sala (RM_0030, 8,93 MB expandida, + 320 KB de `PXLCONV`): ≈ 1,83 MB |
| `Ps2_free_texmemsize` | 7.273.472 B con 1,57 MB de textura de la sala → en la peor sala (RM_4030, 7,39 MB) quedarían ≈ 1,45 MB libres. Las texturas de render (1 MB cada una) no están medidas |
| Memoria por encima de `mempb` que añade P2 | 0 (la coleta no reserva memoria de sala) |

### Checklist de prueba manual

| # | Prueba | Estado |
| --- | --- | --- |
| 1 | P2 lleva el traje de Claire B, con manos y coleta | ✅ (PCSX2, partida cargada) |
| 2 | La coleta de P2 se mueve al andar; la de P1 no cambia | ✅ (PCSX2) |
| 3 | P1 cambia de arma desde el inventario: las manos de P2 cambian y P2 no se mueve | Pendiente (la partida probada solo tiene el mechero) |
| 4 | Partida nueva en la celda: P2 aparece con su traje al recibir el control | Pendiente |
| 5 | Reintentar tras morir y volver al título y cargar: P2 vuelve con su traje | Pendiente (por código: `bhExitGame` libera todas las texturas antes del modo 1) |
| 6 | Cruzar varias puertas: P2 reaparece con traje, manos y coleta | Pendiente |
| 7 | Parte de Chris (cambio de personaje): P2 desaparece y vuelve con Claire | Pendiente |
| 8 | Salas con mucha textura (por ejemplo RM_4030) sin cuelgues | Pendiente |
| 9 | Demo del título: una sola Claire | ✅ (captura del demo con la ISO de la Tarea 3) |
| 10 | P1 con el traje de Claire B (`sys->costume == 1`): P2 carga el traje normal (`[10]`, 8 modelos) y se ve bien | Pendiente |

## Hito 2b: P2 dispara

Spec: [2026-10-10-coop-hito2b-design.md](../superpowers/specs/2026-10-10-coop-hito2b-design.md). Plan: [2026-10-10-coop-hito2b.md](../superpowers/plans/2026-10-10-coop-hito2b.md).

### Implementación

- **Mando:** máscara `0x5FF` (apuntar, arriba/abajo, cambiar de blanco, disparar). Sin acción ni menús. Con armas de mira, P2 no apunta.
- **Combate:** el código original del jugador con `plp = &ply2`. `coopBegin`/`coopEnd` protegen además el bit de arma vacía (compartido con P1), los impactos del frame (`ene[].flg 0x4`), el fogonazo (`rom->lgtp[0]`, fijado en la mano de P2) y el puerto de vibración. Ver [combat.md](../architecture/combat.md).
- **Vibración:** tabla propia del mando 2 en el bloque COOP de `ps2_sg_pdvib.c` (G9), enviada al puerto 1 desde `Coop_pad_read2`. Ver [input.md](../architecture/input.md).
- **Objeto de arma:** `obwp[0]` ↔ `coop_wpn[0]` solo alrededor de `bhControlPlayer()`, para que el disparo de P2 anime su propia arma.
- **Cambio de arma de P1 con P2 apuntando:** P2 vuelve a reposo (`coopStandP2`), limpiando antes el bloqueo de combate (`coopLeaveCombatP2`, también en eventos).
- **Pruebas:** `COOP_TEST` (solo para pruebas) da a P1 una pistola con 15 balas al cargar.

### Checklist de prueba manual (dos mandos)

| # | Prueba | Estado |
| --- | --- | --- |
| 1 | P2 apunta con R1 y dispara con X (configuración A) con la misma arma que P1 | ✅ (usuario, con `RECVX_TEST.iso`) |
| 2 | Los disparos de P2 dañan y matan enemigos | Pendiente |
| 3 | Los disparos de P2 gastan la munición de P1; P2 recarga del inventario de P1 | Pendiente |
| 4 | El fogonazo de P2 ilumina su mano, no la de P1; el arma de P1 no se anima con los disparos de P2 | Pendiente |
| 5 | El mando 2 vibra con los disparos de P2; el mando 1 no | Pendiente |
| 6 | Pausa e inventario con una vibración en curso: ningún mando se queda vibrando | Pendiente |
| 7 | Opción de vibración desactivada: tampoco vibra el mando 2 | Pendiente |
| 8 | P1 y P2 disparan a la vez al mismo enemigo: cuentan los dos impactos | Pendiente |
| 9 | P1 cambia de arma con P2 apuntando: P2 vuelve a reposo con la nueva arma | Pendiente |
| 10 | Escopeta, armas automáticas y armas dobles con P2 | Pendiente |
