# Especificación — Cooperativo, hito 7: P2 de cualquier personaje (7a) y mercenarios a dos (7b)

- **Fecha:** 2026-10-10
- **Estado:** pendiente de revisión del usuario.
- **Decisiones del usuario:**
  - una spec con dos hitos (7a motor, 7b mercenarios) y un plan para cada uno;
  - en mercenarios, **game over si muere cualquiera**;
  - **se puede repetir personaje**;
  - P2 recibe el Linear Launcher igual que P1;
  - **voz propia de P2 ya en el 7a**;
  - en el modo historia **no cambia nada todavía**;
  - la selección de P2 se hace **volviendo a entrar en `RM_5500`**.
- **Depende de:** hitos 2c (arma y bloque propio de P2), 2d (inventario de P2) y 3 (vida de P2, game over).
- **Contexto:**
  - [player.md](../../architecture/player.md#modelo-y-animaciones-del-personaje), [inventory.md](../../architecture/inventory.md), [combat.md](../../architecture/combat.md), [world-systems.md](../../architecture/world-systems.md#driver-de-sonido-iop-y-memoria-spu2);
  - [docs/coop/README.md](../../coop/README.md) (D7, ganchos G1-G21).

## 1. Objetivo

- **7a:** que P2 pueda ser cualquiera de los cinco personajes jugables (Claire, Claire B, Chris, Steve, Wesker) con su modelo, animaciones, armas, sonidos de arma, voz, retrato en la pantalla de estado y comportamiento propio de su personaje (alcance del cuchillo, pasos, tablas de agarre). Es la base para elegir personaje en el modo historia en el futuro, que no forma parte de este hito.
- **7b:** el modo mercenarios (Battle Game, `sys->gm_mode == 3`) para dos jugadores. P1 elige personaje, después P2 elige el suyo, y cada uno empieza con el inventario de mercenarios de su personaje.

## 2. Lo que hay en el original (resumen de la investigación)

- **No hay nada de dos jugadores** en el Battle Game: solo se lee el mando 1.
- **Entrada:**
  - el menú extra del título pone `gm_mode = 3` (adv.c, `CheckButton`, comando 3);
  - `bhFirstGameStart` (system.c:505-513) arranca en la etapa 5, sala 50 (`RM_5500`) con `ply_id = 3`.
- **La selección de personaje es el guion de `RM_5500.RDX`, no código C:**
  - el cursor va en `sys->rm_flg` (bits de índice 5-7), con el orden Claire, Claire B, Chris, Steve, Wesker;
  - los personajes bloqueados se saltan con `bhCk` sobre `sys->ssd_flg`;
  - al confirmar: `bhSv(25, 0/1)` (`sys->costume`, solo para Claire y Claire B), el opcode 0x89 `bhPlayerChangeSet` (`sys->cng_pid = v0; sys->cb_flg |= 0x80`) y el opcode 0xCF `bhExGameItemInit`, que llama a `ExtraGameItemInit` (sub1.c:8567).
- **Cambio de personaje:** lo aplica el cargador, modo 4, pasos 3-7 (system.c:1766-1849):
  1. `ply_id = cng_pid`;
  2. carga el cuerpo `SYSTEM.AFS[ply_id + 10 + costume*4]` y el arma `SYSTEM.AFS[20 + ply_id*30 + wpnr_no]`;
  3. en el Battle Game no hace `bhPushGameData`.
- **Inventario de mercenarios:** `int itemset[5][16]`, local a `ExtraGameItemInit`. La fila es `costume ? 4 : cng_pid`; el elemento [0] es el número de objetos. Escribe en `itm[cng_pid*16 + 2..]` con `(id << 16) | getbulletmax[id][gm_mode]`, sin borrar el bloque ni tocar la casilla equipada. Con `ssd_flg & 0x80000000`, el Linear Launcher (`0x080B0001`) va en la primera casilla.
- **"Menú adaptado":**
  - la pantalla de estado es la misma para todos, con la textura `ITEM.AFS[145]`, que trae los retratos de los 4 personajes;
  - `StatusInit` (sub1.c:1535) elige las piezas con `sys->ply_id`, así que Claire B tiene el retrato de Claire;
  - en el Battle Game, el mapa está bloqueado (sub1.c:4267, system.c:631) y la mochila siempre está activa (sub1.c:3406).
- **Muerte y final:**
  - **muerte:** no hay "Continuar". `bhExitGameOver` hace `bhPopGameData`, con la instantánea que tomó `bhSetPlayer` en la carga completa de `RM_5500`, y se vuelve a la selección;
  - **final:** las salas `RM_5800`…`5840` llaman al ranking;
  - **puntuación:** solo cuenta el tiempo, por `ply_id`/`costume`. Disparar el Linear Launcher (`ev_flg` 75) invalida el récord.
- **`sys->cng_pid` y `sys->costume` están fuera del rango guardado**, así que `bhPopGameData` no los restaura.
- **Dependencia del personaje:** unas 50 lecturas de `sys->ply_id` en player.c (`PlyInfo`, `PlFootSnd`, `KnfAtrTab`, `PlKDU`…). También hay tablas por personaje en los enemigos (`En01_PlyMtn_OffsetTbl[sys->ply_id]` y similares en en01-en26), el bloque `itm[ply_id*16]` en event.c y sub1.c, y la voz `RequestPlayerVoiceSoundBank(sys->ply_id)` (system.c:2274, `CORE_<id>.SPQ`).

## 3. Hito 7a: P2 de cualquier personaje

### 3.1 Comportamiento

1. **Quién es P2:**
   - dos globales nuevas, `coop_p2_id` (0 Claire, 1 Chris, 2 Steve, 3 Wesker) y `coop_p2_cos` (0 o 1, solo con Claire);
   - en el modo historia valen `0` y "el traje que no lleva P1", que es lo de hoy;
   - solo el 7b las cambia.
2. **Modo historia sin cambios:** P2 es Claire con el otro traje y se oculta mientras `sys->ply_id != 0` (D7). La regla de D7 pasa a una función, `coopP2Allowed()`:
   - en el modo historia, `sys->ply_id == 0`;
   - en el Battle Game, siempre (salvo en la sala de selección, ver 4.1).
3. **P2 se comporta como su personaje:**
   - su modelo, sus animaciones de cuerpo y sus armas;
   - su alcance de cuchillo, sus pasos, sus animaciones de daño y su `PlyInfo`;
   - sus animaciones en los agarres;
   - su retrato en su pantalla de estado;
   - sus sonidos de arma y su voz.
4. **Mismo personaje que P1:** se permite. P2 carga su propia copia del cuerpo, igual que hoy con Claire B, porque el árbol de huesos no se puede compartir. Si además es el mismo traje, se ven iguales.

### 3.2 Identidad del personaje en el contexto de P2

Todo el código del jugador lee el personaje de `sys->ply_id` y `sys->costume`. En lugar de tocar esas lecturas, **los ganchos que ya ponen el contexto de P2 cambian también estas dos variables**:

| Dónde | Qué se añade |
| --- | --- |
| `coopBegin` / `coopEnd` (coop.c) | Guardar `sys->ply_id`/`sys->costume`, poner `coop_p2_id`/`coop_p2_cos` y restaurarlos |
| G15 `coopItemselectBegin` / `End` | Lo mismo: el retrato (`StatusInit`, sub1.c:1535) y la lógica de `ply_id == 0` de sub1.c pasan a ser las de P2 |
| G17 `coopEnemyBegin` / `End` (si el objetivo es P2) | Lo mismo: tablas de agarre por personaje (en01.c y demás) |
| G18 `coopEffectBegin` / `End` (si el efecto va a P2) | Nada: ningún efecto lee `ply_id` ni `costume` (comprobado en la implementación) |
| G19 `coopCheckBombP2` | Ya va dentro de `coopBegin`/`coopEnd` |

- **El bloque de inventario de P2 sigue en `itm[256..271]`.** Con `ply_id` cambiado, el código que calcula `itm[ply_id*16]` apuntaría al bloque del personaje, no al de P2. Por eso:
  - G14 sigue fijando `swork.pip` en el bloque de P2;
  - el plan debe revisar las lecturas `itm[sys->ply_id * 16]` de event.c y sub1.c (lista de la sección 2) que puedan ejecutarse dentro del contexto de P2. Las de event.c son de guion (tarea 8), que corre fuera de `coopBegin`/`coopEnd`: hay que comprobarlo.
- **Lo que no se cambia en el contexto de P2:** `sys->ply_stflg[]`, `ply_hp[]` y `ply_wno[]` indexados por `ply_id` (room.c:1030-1039, system.c:1770-1788). Solo los escriben las rutas de cambio de sala y de personaje, que corren fuera de ese contexto. El plan debe confirmar que ninguna corre dentro.

### 3.3 Carga del personaje de P2

1. **Memoria** (`coopInitMemory`, G3, por debajo de `mempb`):
   - **animaciones de cuerpo:** un buffer nuevo `coop_bmt2` y una tabla `coop_mnw2[0..99]` propia, en lugar de la copia de `sys->plmthp` que hace `coopSyncBodyMotions` (G4). Su tamaño es el máximo de las animaciones de cuerpo de los ficheros [10]-[14], que se mide en el plan; el techo es `plbmtp`, 384 KB. Lo mismo para los datos z (`plzmtp`, 8 KB);
   - **modelo:** se agranda `COOP_MODEL_POOL_SIZE` al máximo de los cinco modelos, también medido en el plan;
   - **aunque P2 sea el mismo personaje que P1,** se usan siempre sus buffers propios, para tener un solo camino de código.
2. **Fichero del cuerpo:** `SYSTEM.AFS[coop_p2_id + 10 + coop_p2_cos*4]`, el mismo cálculo que el original.
3. **Arma:**
   - `coopReadWeapon2Data` se lee de `SYSTEM.AFS[20 + coop_p2_id*30 + wpnr_no]`. Claire B usa las de Claire, como en el original;
   - `coopMonitorWeapon2` (G11) usa la misma fórmula.
4. **Objeto enganchado (`sys->obwp[2]` de P1):**
   - Claire usa la coleta (`lkmtab[0]`) y Wesker `lkmtab[1]` (player.c:756-785). Chris y Steve no tienen;
   - la coleta propia de P2 (2a) se generaliza a "objeto enganchado de P2 según `coop_p2_id`".
5. **Sonidos de arma (coopsnd.c):** el banco `ARMS_xxx` de P2 se elige con `WpnTab[...]` del arma de su personaje. Si para Chris, Steve o Wesker la tabla o el banco cambian con `ply_id`, se elige dentro del contexto de P2.
6. **Texturas del cuerpo:** se liberan y se vuelven a pedir al cambiar el personaje de P2, igual que hace el cargador con P1 en el cambio de personaje (system.c:1766-1792).
7. **Cuándo se carga:**
   - en la carga completa (G8, paso 10 del modo 1), como hoy;
   - **y en un gancho nuevo, G22**, en el modo 4 del cargador (cambio de sala), cuando `coop_p2_id`/`coop_p2_cos` no coinciden con lo cargado (`coop_p2_ld_id`, `coop_p2_ld_cos`). G22 va al final de la ruta del cambio de personaje de P1, es decir, tras el paso 7 o en el sitio equivalente si no hay cambio. Reutiliza la máquina de estados de `coopLoadPlayer2` con los buffers reservados en el punto 1, sin reservar memoria nueva.

### 3.4 Voz propia de P2

- **El problema:**
  - la voz del jugador es el banco de SE 4 (puerto 7, `CORE_<id>.SPQ`, 53.760 B en el peor caso), cuya zona de la SPU2 ocupa exactamente ese peor caso;
  - el hueco tras el puerto 7 (0x1E7400, unos 55 KB) ya lo usan los sonidos de arma de P2;
  - queda libre el hueco 0x050A0-0x1521F (unos 64 KB).
- **El diseño:**
  1. Cuando el personaje de P2 es distinto del de P1, se carga `CORE_<coop_p2_id>.SPQ` por la vía de coopsnd (petición propia por `SpqFileReadRequestFlag`).
  2. Sus muestras van al hueco 0x050A0.
  3. Sus programas se añaden al HD del banco 4, como ya se hace con los de arma, en un rango de listas propio.
  4. Un gancho nuevo, **G23**, en la función que pide las voces del jugador (a localizar en sdfunc.c, como G21 con `CallPlayerWeaponSeEx`), lleva las voces pedidas durante el update de P2 a sus programas.
  5. Si P2 es el mismo personaje que P1, usa los programas de P1 sin cargar nada.
- **Prueba previa S2 (antes de implementar):** los desplazamientos de `Vagi` son relativos a la base del puerto 7 (0x1D9C00), y el hueco 0x050A0 está por debajo. Hay que comprobar si `modhsyn` acepta un desplazamiento que, sumado en 24 bits, dé la vuelta hasta 0x050A0, y si el HD del banco 4 (0x1000 B en el IOP) cabe con los programas de voz y de arma de P2 a la vez.
- **Si S2 falla:**
  1. si el HD no cabe, se quitan los programas de voz que P2 no use (los mismos criterios que coopsnd con las armas);
  2. si `modhsyn` no admite el desplazamiento negativo, P2 usa la voz de P1 cuando sea el mismo personaje y no tiene voz cuando no lo sea. Se decide con el usuario antes de seguir.

## 4. Hito 7b: mercenarios a dos

### 4.1 Selección en dos fases

Estado nuevo: `coop_sel` (0 = normal, 1 = P1 ya ha elegido y RM_5500 se repite para P2) y la elección de P1 guardada (`coop_sel_cos1`).

1. **Fase P1** (`coop_sel == 0`): la pantalla original sin cambios. El guion pone `costume`, `cng_pid` y `cb_flg 0x80`, y `ExtraGameItemInit` rellena el bloque de P1.
2. **Paso a la fase P2:**
   - cuando el guion de `RM_5500` pide pasar a la primera sala de combate, un gancho nuevo (**G24**, sitio exacto según la prueba S1) cambia el destino a la misma `RM_5500` y pone `coop_sel = 1`;
   - el cargador aplica el cambio de personaje de P1 (modo 4, `cb_flg 0x80`) durante esa misma carga.
3. **Fase P2** (`coop_sel == 1`): el guion de `RM_5500` se ejecuta otra vez desde el principio.
   - **Mando:** durante toda la fase, el mando 2 hace de mando 1 (`sys->pad_*` toma lo del mando 2) y el mando 1 no hace nada.
   - **G25 (`bhSv`, caso 25, event.c:848):** con `coop_sel == 1`, el valor va a `coop_p2_cos` y `sys->costume` vuelve a `coop_sel_cos1`.
   - **G26 (opcode 0x89, `bhPlayerChangeSet`, event.c:6264):** con `coop_sel == 1`, `coop_p2_id = v0`. No toca `cng_pid` ni `cb_flg`.
   - **G27 (opcode 0xCF, `bhExGameItemInit`, event.c:8906):** con `coop_sel == 1`, llama a `coopBattleItemInit2()` en vez de a `ExtraGameItemInit`.
   - Al empezar la fase P2, `coop_p2_cos = 0`, porque el guion solo lo pone para Claire y Claire B.
4. **Salida:**
   - en la fase P2, G24 deja pasar el cambio a la primera sala de combate y pone `coop_sel = 0`;
   - en esa carga, G22 carga el personaje de P2 (3.3.7).
5. **P2 en `RM_5500`:** oculto en las dos fases. `coopP2Allowed()` da 0 con `sys->rom_no == 50` y `gm_mode == 3`.
6. **Vuelta a la selección:**
   - tras morir, `bhPopGameData` vuelve a la instantánea de la carga completa de `RM_5500`;
   - `coop_sel` se pone a 0 en cualquier entrada a `RM_5500` que no venga de G24, y la selección empieza otra vez por P1.

**Prueba previa S1 (antes de implementar):**
1. Localizar en el guion de `RM_5500` cómo sale de la sala (opcode de cambio de sala, o puerta y `sys->rom_no` de destino) y qué función C lo aplica. Ahí va G24.
2. Comprobar que volver a entrar en `RM_5500` con `ply_id` distinto de 3 repite la selección completa: el guion no depende de `ply_id`, y el estado del cursor (`rm_flg`) se reinicia al entrar.
3. Comprobar si P1 se ve en `RM_5500`. Si se viera tras su cambio de personaje, ocultarlo durante la fase P2.
4. Comprobar qué valor tiene `sys->costume` cuando P1 elige Chris, Steve o Wesker tras haber elegido Claire B en una partida anterior. Si el guion no lo pone a 0, el cargador leería `SYSTEM.AFS[15..17]`, que son ficheros de 11 B; ver si el original lo resuelve en otra parte (por ejemplo, en `scd0`).

Si S1 demuestra que volver a entrar en la sala no repite la selección, se para y se vuelve a decidir con el usuario entre reiniciar el evento del guion o hacer un menú propio.

### 4.2 Inventario de P2

`coopBattleItemInit2()` (coop.c):
1. Pone a cero `itm[256..271]` y mantiene la firma y los metadatos de `itm[272..]`.
2. Usa una copia de la tabla `itemset[5][16]` de `ExtraGameItemInit` en coop.c, porque la original es local. La fila es `coop_p2_cos ? 4 : coop_p2_id`.
3. Con `ssd_flg & 0x80000000`, pone el Linear Launcher (`0x080B0001`) en la primera casilla, como hace el original con P1.
4. Escribe cada objeto como el original: `(id << 16) | getbulletmax[(u8)id][gm_mode]`.
5. Sin arma equipada (`itm[256] = 0`), como P1.
6. La vida de P2 al máximo del modo (`itm[273]`) y sin veneno (`itm[274] = 0`).

### 4.3 Reglas del modo con dos jugadores

| Tema | Comportamiento |
| --- | --- |
| Muerte | La de cualquiera lanza el game over (hito 3). Al volver, `bhPopGameData` restaura la instantánea de `RM_5500` y se elige otra vez (4.1.6) |
| Cronómetro | Uno, el original (`sys->time`). `coopBegin`/`coopEnd` ya conservan `gm_flg 0x80000000` |
| Ranking y récords | Solo de P1 (`ply_id`/`costume` de P1). Si cualquiera dispara el Linear Launcher, `ev_flg` 75 invalida el récord |
| Pantalla de estado de P2 | Su retrato (por el cambio de `ply_id` de 3.2), mapa bloqueado y mochila activa (el original lo hace en `StatusMain`, que también corre para P2) |
| Desbloqueos | Solo los consigue P1, como en el original. Los de P2 no cuentan |
| `costume` de P1 | `bhPopGameData` no lo restaura. Como la vuelta es siempre a la selección, que lo vuelve a poner, solo hace falta si S1.4 lo pide |

## 5. Ganchos nuevos

| Gancho | Dónde | Qué hace | Hito |
| --- | --- | --- | --- |
| G22 | Cargador, modo 4 (cambio de sala), tras el cambio de personaje de P1 (system.c ≈ 1849) | Recarga el cuerpo, las animaciones, el arma y los sonidos de P2 si cambia su personaje | 7a |
| G23 | Función que pide las voces del jugador (sdfunc.c) | Durante el update de P2, sus voces van a sus programas del banco 4 | 7a |
| G24 | Salida de `RM_5500` (sitio según S1) | Fase P1 → vuelve a `RM_5500` en la fase P2; fase P2 → deja pasar | 7b |
| G25 | `bhSv`, caso 25 (event.c:848) | En la fase P2, el traje va a `coop_p2_cos` | 7b |
| G26 | Opcode 0x89 `bhPlayerChangeSet` (event.c:6264) | En la fase P2, el personaje va a `coop_p2_id` | 7b |
| G27 | Opcode 0xCF `bhExGameItemInit` (event.c:8906) | En la fase P2, `coopBattleItemInit2` | 7b |

**Ganchos ya existentes que se cambian:**
- **G3:** buffers de animación y modelo de P2.
- **G4:** deja de copiar la tabla de animaciones de P1.
- **G8:** fichero según `coop_p2_id`/`cos`; en el Battle Game no carga hasta G22.
- **G11:** banco de armas por personaje.
- **G15, G17 y G18:** cambio de `ply_id`/`costume`.
- **D7** pasa a `coopP2Allowed()`.

Todo va detrás de `#ifdef COOP`. Sin `COOP`, el ejecutable debe salir idéntico a la línea base (objdiff y comparación de segmentos `PT_LOAD`).

## 6. Errores y robustez

- **Memoria:** si el cuerpo o las animaciones del personaje de P2 no caben en los buffers, o el pool de texturas no tiene sitio, P2 no se carga. Se queda oculto, con un `printf("[COOP] ...")`, y la partida sigue con P1 solo, como hoy cuando falla la carga.
- **Mediciones del plan:**
  - tamaños de modelo y animaciones de [10]-[14];
  - margen de RAM y del pool de texturas con P1 y P2 en la peor combinación (los dos personajes con más textura) en la peor sala de mercenarios, con `ramread.py`.
- **Fichero de 11 B o vacío** (traje inexistente): no se carga. Se usa el traje 0 de ese personaje.
- **Recarga a mitad de un sonido de P2:** G22 espera a `CheckTransEndSoundBank() == 0` antes de pedir los bancos de P2, como hace `coopSeStep`.
- **Demo de atracción:** P2 sigue desactivado (`coopDemo()`).
- **Fase P2 interrumpida** (por ejemplo, volver al título): `coop_sel` se pone a 0 en `bhFirstGameStart` y en cada entrada a `RM_5500` que no venga de G24.

## 7. Verificación

Sin tests automáticos. Compilar sin errores, generar `RECVX_NEW.iso` y probar en PCSX2 con dos mandos ([testing.md](../../coop/testing.md)).

**7a:**

| # | Prueba |
| --- | --- |
| 1 | Sin `COOP`: el ELF es idéntico a la línea base |
| 2 | Modo historia: P2 sigue siendo Claire B (o A), se oculta con Chris y vuelve con Claire. Ninguna diferencia con el hito 3 |
| 3 | Con `COOP_TEST` y un `coop_p2_id` forzado (1, 2 y 3) en el modo historia: P2 aparece como Chris, Steve o Wesker, anda, corre, apunta y dispara con sus animaciones; el cuchillo y los pasos son los suyos |
| 4 | El inventario de P2 muestra su retrato |
| 5 | P2 recibe daño y grita con su voz; P1 sigue con la suya |
| 6 | Un zombi agarra a P2 (Chris o Steve) y la animación del agarre encaja |

**7b:**

| # | Prueba |
| --- | --- |
| 7 | Mercenarios: P1 elige con el mando 1; la pantalla se repite y P2 elige con el mando 2 (el mando 1 no responde) |
| 8 | Cada uno aparece con su personaje y su inventario de mercenarios, incluida la repetición (Claire y Claire) |
| 9 | Con el Linear Launcher desbloqueado, los dos lo tienen |
| 10 | Muere P2 → game over → vuelta a la selección empezando por P1 |
| 11 | Llegar al final con los dos: el ranking es el de P1 |
| 12 | La peor combinación de memoria (medida) carga en todas las salas de mercenarios |

## 8. Fuera de alcance

- Elegir el personaje de P2 o de P1 en el modo historia, y que P2 aparezca en las partes de Chris (sigue D7). El mecanismo del 7a lo deja preparado.
- Cámara para dos (hito 5): en mercenarios la cámara sigue a P1, como en la historia.
- Que P2 desbloquee personajes o récords.
- Ranking o puntuación por jugador.
- Elegir personaje para los dos a la vez en la misma pantalla.
- Que el que sobrevive siga solo cuando muere el otro.
