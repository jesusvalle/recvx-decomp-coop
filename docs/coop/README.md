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
| 2026-10-11 | **Hito 2c implementado** (sin commits, sin probar en PCSX2): P2 lleva su propia arma (el cuchillo al empezar), cargada desde su bloque de inventario `itm[256..]`, con sus modelos, texturas, animaciones y munición. Plan: [2026-10-11-coop-hito2c.md](../superpowers/plans/2026-10-11-coop-hito2c.md). Sin `COOP`, idéntico a la línea base. Revisión de la sesión: [revision-2026-10-11.md](revision-2026-10-11.md). Revisión final: sin críticos ni importantes. |
| 2026-10-11 | **Hito 2d implementado** (sin commits, sin probar en PCSX2): Start en el mando 2 abre el inventario de P2, que usa, combina, examina y equipa con su mando; P2 recoge objetos con su botón de acción y usa el baúl general compartido. Plan: [2026-10-11-coop-hito2d.md](../superpowers/plans/2026-10-11-coop-hito2d.md). Revisión final: un fallo crítico (las pulsaciones de P2 fuera de su máscara se repetían cada frame) y tres importantes, corregidos. |
| 2026-10-10 | **Sonidos de arma propios de P2** (sin commits): los sonidos del arma de P2 salen de su propio banco (`ARMS_xxx`), reducido a lo que usa el jugador y añadido al banco de voz, con las muestras en un hueco libre de la RAM de sonido. Antes, con P1 con el mechero o la pistola y P2 con el cuchillo, el cuchillo de P2 no sonaba. Probado en PCSX2 con la partida de prueba (P1 mechero, P2 cuchillo): HD fusionado en el IOP, muestras en la SPU2 y voces de la SPU2 leyendo de ellas al dar P2 una cuchillada; falta oírlo. Ver [Sonidos de arma de P2](#sonidos-de-arma-de-p2). |
| 2026-10-11 | **Hito 3 implementado** (sin commits, sin probar en PCSX2): P2 tiene vida propia (guardada con la partida), los enemigos comunes van a por el jugador más cercano y le golpean, agarran y muerden; ácido, fuego, gas y explosiones dañan también a P2; si muere cualquiera de los dos, game over. Plan: [2026-10-11-coop-hito3.md](../superpowers/plans/2026-10-11-coop-hito3.md). Revisión final: un fallo crítico (P2 muerto en un agarre dejaba al enemigo y a P2 congelados) y cuatro importantes, corregidos. |
| 2026-10-10 | **Hito 7 implementado** (sin commits, sin probar en PCSX2): 7a, P2 puede ser cualquier personaje (cuerpo, animaciones, datos z, armas, objeto enganchado, voz y retrato propios; dentro de sus contextos `sys->ply_id`/`costume`/`plzmtp` son los suyos); 7b, mercenarios a dos (P1 elige, la sala de selección se repite para P2 con el mando 2, cada uno con su inventario de mercenarios). En la historia no cambia nada (ni se carga la voz de P2). Revisión final: ningún fallo crítico; tres importantes (posición al repetir la selección, voz cargada sin necesidad en la historia, bloqueo sin mando 2) y cuatro menores, corregidos. Sin `COOP`, idéntico a la línea base. Spec: [2026-10-10-coop-hito7-design.md](../superpowers/specs/2026-10-10-coop-hito7-design.md). Plan: [2026-10-10-coop-hito7.md](../superpowers/plans/2026-10-10-coop-hito7.md). Ver [Hito 7](#hito-7-p2-de-cualquier-personaje-y-mercenarios-a-dos). |

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
- [x] Munición compartida con P1, incluida la recarga
- [x] Los disparos de P2 dañan a los enemigos
- [x] El fogonazo sale en la mano de P2
- [x] El arma de P1 no se anima con los disparos de P2 (sin probar)
- [x] Dos impactos en el mismo enemigo y frame cuentan los dos (sin probar)
- [x] Las vibraciones de P2 van al mando 2
- [x] Armas con mira bloqueadas para P2
- [x] Si P1 cambia de arma con P2 apuntando, P2 vuelve a reposo y puede volver a apuntar

### Hito 2c — Arma propia de P2

Spec: [2026-10-11-coop-hito2c-design.md](../superpowers/specs/2026-10-11-coop-hito2c-design.md).

- [x] Bloque de inventario de P2 en `sys->itm[256..271]` (se guarda con la partida sin cambiar el formato), sembrado con el cuchillo (sin probar)
- [x] Cargador de arma propio: modelos de manos y arma, animaciones y texturas de P2 (sin probar)
- [x] Tabla de animaciones propia (cuerpo copiado de P1, arma propia) (sin probar)
- [x] Munición propia y aviso de arma vacía por jugador (sin probar)
- [x] P1 y P2 con armas distintas a la vez (sin probar)
- [x] Los sonidos del arma de P2 salen de su propio banco (G20, G21; ver [Sonidos de arma de P2](#sonidos-de-arma-de-p2))

### 🟡 Hito 2d — Inventario de P2, recoger objetos y baúl

Spec: [2026-10-11-coop-hito2d-design.md](../superpowers/specs/2026-10-11-coop-hito2d-design.md).

- [x] Start en el mando 2 abre el inventario de P2 (el juego se pausa para los dos) (sin probar)
- [x] Usar, combinar, examinar y equipar desde la pantalla de P2 (equipar usa el cargador de 2c) (sin probar)
- [x] P2 recoge objetos con su botón de acción, a su propio inventario (sin probar)
- [x] Baúl general compartido entre P1 y P2 (sin probar)
- [x] Objetos clave en P2 bloqueados para usar hasta el hito 4 (sin probar)

### 🟡 Hito 3 — Salud de P2 y enemigos que le atacan

Spec: [2026-10-11-coop-hito3-design.md](../superpowers/specs/2026-10-11-coop-hito3-design.md).

- [x] Vida propia de P2, guardada con la partida (cojea con poca vida) (sin probar)
- [x] Cada enemigo persigue al jugador más cercano, sin cambiar de objetivo en mitad de un ataque o agarre (sin probar)
- [x] Agarres y mordiscos sobre P2, que se suelta con el mando 2 (sin probar)
- [x] Ácido, gas, fuego y explosiones dañan también a P2 (sin probar)
- [x] Game over si muere cualquiera de los dos, con el reintento normal (sin probar)

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

### 🟡 Hito 7 — Otros personajes y modos

Spec: [2026-10-10-coop-hito7-design.md](../superpowers/specs/2026-10-10-coop-hito7-design.md).

- [x] 7a: P2 de cualquier personaje (Claire, Claire B, Chris, Steve, Wesker), con su modelo, animaciones, armas, voz y retrato (sin probar)
- [x] 7b: mercenarios a dos (P1 y luego P2 eligen personaje; cada uno con su inventario de mercenarios) (sin probar)
- [ ] P2 durante la parte de Chris en la historia (usa el 7a)
- [ ] Activar o desactivar el cooperativo desde el juego (sin recompilar)

## Decisiones tomadas

| # | Decisión | Por qué |
| --- | --- | --- |
| D1 | **En el hito 1, P2 es un clon de Claire.** Comparte texturas y animaciones con P1; tiene su propio árbol de huesos, sus matrices y sus bloques `exp`. | Es lo más barato: no hace falta un segundo cargador de personaje ni ~1.7 MB extra (buffers + texturas). Decidido por el usuario el 2026-10-09. |
| D2 | **Enfoque de "cambio de contexto":** P2 es un segundo `BH_PWORK` estático (`ply2`). En cada frame, después de `bhControlPlayer()`, se hace `plp = &ply2`, se intercambia el estado del mando y se vuelve a llamar a `bhControlPlayer()`. Después se restaura todo. | Todo player.c trabaja sobre el global `plp` y lee `sys->pad_*`. Así P2 tiene todo el repertorio de movimientos (y más adelante apuntar, disparar y recibir daño) sin reescribir ~7.300 líneas. Se descartó un controlador propio para P2 porque habría que tirarlo en cuanto P2 necesite combatir. |
| D3 | **Todo el código cooperativo va detrás de `#ifdef COOP`**, preferiblemente en archivos nuevos (por ejemplo `coop.c`/`coop.h`). | Compilando sin `COOP`, el código del juego sale igual que sin el mod. Eso permite comparar con objdiff y no ensucia el código decompilado. |
| D5 | **El hito 2 se divide en 2a (aspecto), 2b (disparo) y 2c (inventario).** | El disparo con el mismo tipo de arma que P1 es barato; equipar un arma distinta (lo que da sentido al inventario propio) exige un cargador de arma propio. Decidido por el usuario el 2026-10-10. |
| D6 | **P2 lleva el traje que no lleva P1** (Claire B si P1 va con el normal) **y en las manos la misma arma que P1** (clon visual de `sys->obwp[0/1]`). | Las animaciones de los dos trajes son idénticas, así que P2 solo necesita su modelo y sus texturas. Las manos van con el arma; clonar la de P1 es la base del disparo de 2b. Decidido por el usuario el 2026-10-10. |
| D7 | **En la historia, P2 se oculta mientras P1 no sea Claire** (`sys->ply_id != 0`, ahora en `coopP2Allowed`). En mercenarios, P2 está siempre, salvo en la sala de selección. | Hasta el 7a, P2 usaba las animaciones de P1. Desde el 7a ya podría ir con Chris, pero los guiones de la parte de Chris no cuentan con P2; se deja para más adelante. Decidido por el usuario el 2026-10-10 (y mantenido en el 7a). |
| D8 | **En 2b, P2 comparte la munición de P1** (`swork.pip` no cambia). **Desde el 2c, cada uno gasta la de su bloque.** | Es lo más equilibrado sin inventario propio; en 2c pasa a ser suya. Decidido por el usuario el 2026-10-10. |
| D9 | **Las vibraciones de P2 van al mando 2.** | Tabla de vibración propia de P2 enviada al puerto 1; antes iban todas al mando 1. Decidido por el usuario el 2026-10-10. |
| D10 | **Orden del hito 2 en adelante: 2c arma propia → 2d inventario, recoger y baúl → 3 salud y enemigos.** | El cuchillo inicial y equipar desde la pantalla de P2 necesitan antes el cargador de arma propio. Propuesto al usuario el 2026-10-11. |
| D11 | **El inventario de P2 vive en `sys->itm[256..271]`, con metadatos (firma, vida, veneno) en `itm[272..279]`.** | Rango libre en el código y en los 205 guiones de sala, y dentro de la partida guardada: se guarda sin cambiar el formato. |
| D12 | **P2 empieza con el cuchillo; cada jugador recoge a su propio inventario; el baúl general es compartido.** | Decidido por el usuario el 2026-10-11. |
| D13 | ~~**Los disparos de P2 suenan con el banco de sonidos del arma de P1.**~~ Sustituida por D16. | Solo puede haber un banco de armas cargado. Decidido por el usuario el 2026-10-11. |
| D14 | **Los enemigos van a por el jugador más cercano; game over si muere cualquiera de los dos.** | Decidido por el usuario el 2026-10-11. |
| D16 | **Los sonidos del arma de P2 van al banco de SE 4 (voz)**, como programas 32 + lista, con las muestras de su `ARMS_xxx` en el hueco libre de la SPU2 que empieza en 0x1E7400. Solo se suben las que usa el jugador (y las demás mientras quepan, hasta 0xDDC0 bytes). | El IOP solo admite un banco por puerto y un banco de armas completo no cabe; el hueco lo dejó la reserva de reverb (el juego solo usa Hall). No toca el driver del IOP. Pedido por el usuario el 2026-10-10. |
| D15 | **El botón de acción de P2 solo mira zonas de objeto y de baúl general**, con un sondeo propio (`coopActionP2`) que copia la prueba de posición y ángulo del tipo 4 de `bhCheckExmAtari`; no llama a `bhCheckExmAtari`. | Así P2 no abre puertas, no sube escaleras ni salientes y no examina (hito 4), sin tocar las zonas de la sala. Tomada sin el usuario durante el 2d (pendiente de su revisión). |
| D17 | **P2 puede ser cualquier personaje (7a):** dentro de sus contextos (`coopBegin`/`coopEnd`, G15 y G17), `sys->ply_id`, `sys->costume` y `sys->plzmtp` pasan a ser los de P2. Carga sus propias animaciones de cuerpo y datos z, y su arma del banco de su personaje. | Casi todo lo que depende del personaje (alcance del cuchillo, pasos, tablas de agarre, retrato) lee `sys->ply_id`: cambiarlo en el contexto evita tocar el código original. Decidido por el usuario el 2026-10-10. |
| D18 | **Selección de mercenarios en dos fases repitiendo `RM_5500`:** al confirmar P1, la puerta a 5-52 se redirige a 5-50 y el guion de selección se repite con el mando 2; en esa fase el traje, el personaje y el inventario van a P2. Se puede repetir personaje; P2 recibe el Linear Launcher si está desbloqueado; game over si muere cualquiera; cronómetro, ranking y desbloqueos, de P1. | No hay código C de selección (es el guion de la sala). Repetir la sala no exige entender ni parchear el guion. Decidido por el usuario el 2026-10-10. |
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
  | G4 | ~~Final de `bhReadPlayerData` (dread.c)~~ | Quitado en el hito 7 (P2 carga sus animaciones). Fue `coopSyncBodyMotions` (2c), `coopCloneModel` (hito 1) y `coopCloneWeapon` al final de `bhReadWeaponData` (2a-2b) |
  | G5 | `bhFinishRoom` | `coopRoomStart` |
  | G6 | `bhMainSequence` | `coopControlPlayer2` |
  | G7 | `bhAllDrawModel` | `coopDrawPlayer2` |
  | G8 (hito 2a) | `bhSysCallMonitor`, modo 1, paso 10 | `if (coopLoadPlayer2() == 0) break;` |
  | G9 (hito 2b) | `pdVibMxIsReady`, `pdVibMxStart`, `pdVibMxStop` (ps2_sg_pdvib.c) | Puerto lógico 8 → vibración del mando 2 (`coopVibIsReady2`, `coopVibStart2`, `coopVibStop2`) |
  | G11 (hito 2c) | `bhSysCallMonitor`, principio de `case 3` (system.c) | Con `mn_md3 == COOP_MN_P2`: `coopMonitorWeapon2` (carga el arma de P2) |
  | G12 (hito 2c) | `bhEff007`, `case 0` (effsub1.c) | `coopEff007Mag(op)`: el cargador de las armas 12/13 se oculta en el arma del tirador |
  | G13 (hito 2d) | `bhSysCallGame`, alrededor de `bhCheckSubTask` (system.c) | `coopPreSubTask` / `coopPostSubTask`: el Start o la petición de P2 abre su pantalla |
  | G14 (hito 2d) | `StatusMain`, `case 0x1`, antes de `CursorInit` (sub1.c) | `coopStatusInit`: la pantalla de P2 usa `itm[256..]` |
  | G15 (hito 2d) | `bhSysCallItemselect`, alrededor de su cuerpo (system.c) | `coopItemselectBegin` / `coopItemselectEnd`: `plp = &ply2`, equipar con el cargador de P2, cierre |
  | G16 (hito 2d) | Principio de `ItemUse` (sub1.c) | `coopItemUseBlocked`: los objetos para activadores de suelo no se usan desde la pantalla de P2 |
  | G17 (hito 3) | `bhControlEnemy`, alrededor de `bhJumpEnemy[ep->id](ep)` (eneset.c) | `coopEnemyBegin` / `coopEnemyEnd`: elige objetivo; si es P2, `plp`, mando y vibración de P2 durante el update del enemigo |
  | G18 (hito 3) | `bhControlEffect`, alrededor del despacho por id (effect.c) | `coopEffectBegin` / `coopEffectEnd`: los efectos dañinos van al jugador más cercano; gas de sala sobre P2 |
  | G19 (hito 3) | Principio de `bhCheckBombAtari` (weapon.c) | `coopCheckBombP2`: las explosiones dañan también a P2 |
  | G20 (sonido) | `LoadSoundPackFile`, principio de `case 2` (sdfunc.c) | `coopSePack`: el banco de armas de P2 va al banco 4 con su BD en el hueco; al banco de voz se le añaden los programas de P2 |
  | G21 (sonido) | `CallPlayerWeaponSeEx`, tras calcular `NeoSlotNo` (sdfunc.c) | `coopWeaponSeNo`: durante el update de P2, banco 4 y lista 32 + lista |
  | G22 (hito 7a) | `bhSysCallMonitor`, modo 4, principio del paso 10 (system.c) | `coopReloadPlayer2`: en mercenarios, carga a P2 (cuerpo, arma, sonidos) en la primera sala tras elegir y vuelve a llamar a `coopRoomStart` |
  | G23 (hito 7a, sonido) | Principio de `CallPlayerVoice` (sdfunc.c) | `coopVoiceSeNo`: durante el update de P2 (o de un enemigo que va a por P2), sus voces van a las listas 4-7 del banco 4 si su personaje no es el de P1 |
  | G28 (hito 7a, sonido) | `sdBankDownload`, caso `SDE_DATA_TYPE_SHOT_BANK`, tras `iop_trans_offset = 0` (ps2_sg_sd.c) | `coopSeBdOffset`: la subida del BD de la voz de P2 empieza en 0x2B4A0 (con la vuelta de los 2 MB cae en el hueco bajo 0x050A0) |
  | G24 (hito 7b) | `bhStartDoorDemo`, `case 0`, antes de aplicar el destino (room.c) | `coopBattleDoor`: la salida de la selección (5-50 → 5-52) vuelve a 5-50 la primera vez (fase P2) y deja pasar la segunda |
  | G25 (hito 7b) | `bhSv`, caso 25 (event.c) | `coopBattleCostume`: en la fase P2, el traje es de P2 |
  | G26 (hito 7b) | `bhPlayerChangeSet`, opcode 0x89 (event.c) | `coopBattleChange`: en la fase P2, el personaje es de P2 (sin `cng_pid` ni `cb_flg 0x80`) |
  | G27 (hito 7b) | `bhExGameItemInit`, opcode 0xCF (event.c) | `coopBattleItemInit`: en la fase P2, el inventario de mercenarios va al bloque de P2 |

  No hay G10 (la numeración de las specs del 2c lo saltó).

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
- los enemigos ignoren a P2 (P2 sí se aparta de ellos: su propio update llama a `bhCheckEnemies`);
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
| 2 | Los disparos de P2 dañan y matan enemigos | ✅ (usuario) |
| 3 | Los disparos de P2 gastan la munición de P1; P2 recarga del inventario de P1 | ✅ (usuario) |
| 4 | El fogonazo de P2 ilumina su mano, no la de P1; el arma de P1 no se anima con los disparos de P2 | ✅ fogonazo (usuario); falta confirmar el arma de P1 |
| 5 | El mando 2 vibra con los disparos de P2; el mando 1 no | ✅ (usuario) |
| 6 | Pausa e inventario con una vibración en curso: ningún mando se queda vibrando | Pendiente |
| 7 | Opción de vibración desactivada: tampoco vibra el mando 2 | Pendiente |
| 8 | P1 y P2 disparan a la vez al mismo enemigo: cuentan los dos impactos | Pendiente |
| 9 | P1 cambia de arma con P2 apuntando: P2 vuelve a reposo con la nueva arma | ✅ (usuario: vuelve a apuntar) |
| 10 | Escopeta, armas automáticas y armas dobles con P2 | Pendiente |

## Hito 2c: arma propia de P2

Spec: [2026-10-11-coop-hito2c-design.md](../superpowers/specs/2026-10-11-coop-hito2c-design.md). Plan: [2026-10-11-coop-hito2c.md](../superpowers/plans/2026-10-11-coop-hito2c.md).

### Implementación

- **Bloque de P2:** `sys->itm[256..271]` con el formato de un personaje, más firma, vida y veneno en `itm[272..274]` (ver [inventory.md](../architecture/inventory.md)). Se siembra con el cuchillo si falta la firma (partida nueva, partida sin el mod).
- **Cargador de arma propio:** `coopReadWeapon2Data` lee `SYSTEM.AFS[20 + wpnr_no]` en la carga completa (G8, tras el cuerpo) y en el modo 3 del cargador cuando lo pida el inventario de P2 (G11, hito 2d). Modelos y `owP` en `coop_wmdl2`, animaciones en `coop_wmt2` y `coop_mnw2[100..]`, texturas propias que solo libera P2. Ver [combat.md](../architecture/combat.md).
- **Animaciones del cuerpo:** `coop_mnw2[0..99]` se copia de la tabla de P1 al final de cada `bhReadPlayerData` (G4).
- **Munición y bits por jugador:** `coopBegin` pone `swork.pip = &sys->itm[256]` y los bits `gm_flg 0x40000`/`0x10000000` de P2; `coopEnd` los guarda en `coop_gm2` y restaura los de P1.
- **G12:** el efecto del cargador de las armas 12/13 escribe en el arma de su tirador.
- **`COOP_TEST`** (solo pruebas): pone una pistola con 15 balas, equipada, en el bloque de P2.
- **Memoria:** unos 108 KB más bajo `mempb` (176.384 B en total para P2); margen calculado en RM_0030 ≈ 1,72 MB. Cada arma de P2 ocupa unos 135-205 KB del pool de texturas (sin medir en juego); la comprobación previa pide el doble (cuenta los bloques enteros del fichero).

### Checklist de prueba manual

| # | Prueba | Estado |
| --- | --- | --- |
| 1 | Cargar la partida: P2 aparece con el cuchillo en la mano; P1 con su mechero y su luz | Pendiente |
| 2 | P2 apunta (R1) y da cuchillazos (X) con el mando 2 | Pendiente |
| 3 | Con `RECVX_TEST.iso`: P2 aparece con la pistola; dispara y la munición baja en su bloque, no en el de P1 | Pendiente |
| 4 | P1 y P2 con armas distintas a la vez | Pendiente |
| 5 | P1 cambia de arma desde su inventario: P2 no cambia de arma ni pierde texturas | Pendiente |
| 6 | Cruzar puertas: P2 conserva su arma | Pendiente |
| 7 | Reintentar tras morir y volver al título y cargar: P2 vuelve con su arma | Pendiente |
| 8 | Cambio de personaje y vuelta a Claire: P2 vuelve y se anima bien | Pendiente |
| 9 | Partida nueva: P2 empieza con el cuchillo | Pendiente |

## Hito 2d: inventario de P2, recoger objetos y baúl

Spec: [2026-10-11-coop-hito2d-design.md](../superpowers/specs/2026-10-11-coop-hito2d-design.md). Plan: [2026-10-11-coop-hito2d.md](../superpowers/plans/2026-10-11-coop-hito2d.md).

### Implementación

- **Dueño de la pantalla:** la pantalla de inventario es la original (`StatusMain`, tarea 9) con un dueño (`coop_inv_owner`). G13 inyecta el Start de P2 (`pad_ps |= 0x4000`) o su petición (`cb_flg 0x10/0x20000/0x40000`, con su `sb_id` y su `etc_idx`) antes de `bhCheckSubTask`; si la pantalla se abre (`st_flg & 0x8`), es de P2. Si no, se deshace. P1 tiene prioridad.
- **Con la pantalla de P2:** `sys->pad_*` es el mando 2 sin máscara (G2), `swork.pip` es `itm[256..]` (G14), `plp = &ply2` solo alrededor de `ItemTaskCheck`/`StatusMain` (G15) y `sb_id`/`etc_idx`/`cb_flg 0x100` son los de la petición de P2 (así `bhItmCk` quita del mundo el objeto que coge). Equipar marca el modo 3 con `COOP_MN_P2` (cargador del 2c). El crítico de P1 (`gm_flg 0x10000000`) se protege.
- **Cierre:** un único punto, `coopItemselectEnd`, cuando `ts_flg & 0x200` y no se va al mapa: repone `swork.pip`, `sb_id`, `etc_idx`, `cb_flg 0x100` y el mando de P1 (para que `bhSetPad` siga calculando bien sus pulsaciones).
- **Acción de P2** (D15): con el mando 2 y P2 en reposo, `coopActionP2` busca una zona de objeto o de baúl general delante de P2. Objeto: se agacha si la zona lo pide (`bhCPM2_act_cro`) y la petición sale al terminar; baúl: directamente o con la tapa (`bhObjItmBox`, cuya petición se reconoce como de P2). Los baúles especiales A y B siguen siendo de P1. Una petición que no se puede abrir en 5 s se descarta y P2 se levanta.
- **Objetos clave** (G16): desde la pantalla de P2, lo que iría a un activador de suelo (`Use_01`/`Use_05`) muestra el mensaje 160 y no se usa.

### Checklist de prueba manual

| # | Prueba | Estado |
| --- | --- | --- |
| 1 | Start en el mando 2: se abre el inventario con los objetos de P2 (cuchillo), manejado con el mando 2; el mando 1 no lo mueve | Pendiente |
| 2 | Cerrar el inventario de P2 y seguir jugando: P1 y P2 se mueven bien; Start en el mando 1 abre el de P1 (mechero) | Pendiente |
| 3 | Con `RECVX_TEST.iso`: P2 equipa el cuchillo o la pistola desde su pantalla y su arma cambia; la de P1 no | Pendiente |
| 4 | Mapa desde la pantalla de P2 y vuelta: sigue siendo la de P2 | Pendiente |
| 5 | P2 recoge un objeto de la sala con su botón de acción: desaparece del mundo y aparece en su inventario, no en el de P1 | Pendiente |
| 6 | P2 abre el baúl general, deja un objeto; P1 lo saca | Pendiente |
| 7 | P2 delante de una puerta, escalera o algo examinable: el botón de acción no hace nada | Pendiente |
| 8 | Inventario de P2 lleno: no coge el objeto y se levanta | Pendiente |
| 9 | Objeto clave en el inventario de P2: "usar" muestra un mensaje y no hace nada | Pendiente |
| 10 | Guardar en la máquina de escribir con objetos en P2 y volver a cargar: P2 los conserva | Pendiente |
| 11 | Mantener X o Start un rato en la pantalla de P2: cada pulsación cuenta una vez (no se repite ni se reabre al cerrar con Start) | Pendiente |
| 12 | P1 examina algo (mensaje en pantalla) y P2 pulsa acción delante de un objeto: P2 espera a que acabe el mensaje | Pendiente |
| 13 | P1 y P2 abren a la vez el mismo baúl con tapa: se abre una sola pantalla y ninguno se queda congelado | Pendiente |

## Hito 3: salud de P2 y enemigos que le atacan

Spec: [2026-10-11-coop-hito3-design.md](../superpowers/specs/2026-10-11-coop-hito3-design.md). Plan: [2026-10-11-coop-hito3.md](../superpowers/plans/2026-10-11-coop-hito3.md).

### Implementación

- **Vida:** ya no es fija. `itm[273]` (vida) e `itm[274]` (veneno) se escriben al final de cada update de P2 y al cerrar su pantalla, y se leen en `coopRoomStart` (con `dmlvl` recalculado: cojea con poca vida). Así viajan con el reintento, la máquina de escribir y la tarjeta.
- **Enemigos** (G17): tabla `coop_tgt[128]`. Los enemigos comunes (zombis 1 y 26, arañas 3/23/24, Cerberus 4, Hunter 5, murciélagos 7, Bandersnatch 9, 10, Albinoid 21/22, 30) eligen al jugador vivo y visible más cercano (preferencia por el mismo piso; cambio solo si el otro está a menos del 75 % y han pasado 30 frames). No cambian mientras no están en su estado normal (`mode0 != 1`) o mientras su objetivo está siendo golpeado o agarrado. Las partes enganchadas siguen a su dueño. Si el objetivo es P2, el update del enemigo corre con `plp = &ply2`, el mando de P2 y `CurrentPortId = 1`. Los demás (jefes, trampas, polilla, Spotter, grúa), siempre P1.
- **Efectos** (G18): ácido (256), polvo de polilla (260), fuego de Alexia (265, 266, 269), vómito (350) y gas de Nosferatu (397) van al jugador más cercano. Gas de sala (127): se compara la cabeza de P2 con `sys->gas_py`.
- **Explosiones** (G19): el bloque del jugador de `bhCheckBombAtari` se repite sobre P2.
- **Bloqueo primero:** un enemigo ocupado (fuera de `mode0 1`) o con su objetivo golpeado o agarrado conserva el objetivo aunque P2 haya muerto en el agarre; P2 muerto u oculto solo se excluye de las elecciones nuevas.
- **Mientras P2 muere** (hasta que empieza el game over), el mando de P1 se anula, como el original bloquea al jugador que muere: si no, P1 podría cruzar una puerta y la sala nueva resucitaría a P2.
- **Muerte:** la lanza el update de P2 (`bhCPM0_die`/`enedie`, que activan la tarea de game over). `coopPlaceNearP1` y `coopStandP2` no reaniman a P2 muerto; si muere oculto, se fuerza su animación de muerte; muriendo no se congela con mensajes. `coopBegin`/`coopEnd` protegen los fundidos de P1 (`fade_an/rn/gn/bn`).

### Checklist de prueba manual

| # | Prueba | Estado |
| --- | --- | --- |
| 1 | Un zombi va a por el jugador más cercano; si los dos se separan, cambia de objetivo | Pendiente |
| 2 | Un zombi agarra a P2: el mando 2 vibra y machacando botones en el mando 2 se suelta; P1 sigue libre | Pendiente |
| 3 | P2 con poca vida cojea; se cura desde su inventario | Pendiente |
| 4 | P2 muere: game over normal; "Continuar" vuelve con los dos y la vida que tenían en el punto de control | Pendiente |
| 5 | Una explosión cerca de los dos daña a los dos | Pendiente |
| 6 | Ácido de araña, polvo de polilla o fuego dañan a P2 si está más cerca | Pendiente |
| 7 | Guardar con P2 herido y cargar: P2 vuelve herido | Pendiente |
| 8 | Hunter y Cerberus atacan a P2; los jefes siguen yendo a por P1 | Pendiente |

## Sonidos de arma de P2

Código: [coopsnd.c](../../src/ps2/veronica/prog/coopsnd.c), ganchos G20 y G21 en [sdfunc.c](../../src/ps2/veronica/prog/sdfunc.c). La investigación del driver y de la memoria de sonido está en [world-systems.md](../architecture/world-systems.md#driver-de-sonido-iop-y-memoria-spu2).

- **Carga:** cuando P2 carga un arma (`coopLoadPlayer2`, paso 6, y `coopMonitorWeapon2`, paso 2), `coopSeLoad(WpnTab[arma].snd_wpno)` y `coopSeStep` piden su `ARMS_xxx.SPQ` por la vía normal, con `SpqFileReadRequestFlag = 5`. Así se serializa con las cargas de P1, que esperan a que el flag valga 0. El cargador espera a que termine.
- **G20** (en la interrupción de VSync, como toda la carga de bancos):
  - si el `.SPQ` es el de P2, `coopSeBuildP2` elige los programas. Primero van las listas del jugador (disparo, bombeo, cargador, corredera, sin munición, casquillo, cuchillo…) y después las demás, mientras quepan en 0xDDC0 bytes;
  - compacta sus muestras al principio del BD y renumera muestra, sample y sample set con el mismo índice, que es lo que espera el driver;
  - `coopSeMerge` las junta con el HD de voz guardado, con los programas de P2 en 32 + lista;
  - el bloque se reescribe para que vaya al banco 4. Mientras se sube, `SE_BANK[4] = 8`: `Tsnd_spuadr_tbl[8]` = 0x1E7400, el hueco;
  - si el `.SPQ` es el de voz (`CORE_xxx`), se guarda su HD y se le añaden los programas de P2.
- **G21:** con `plp == &ply2` y el banco listo, `CallPlayerWeaponSeEx` pide banco 4 y lista 32 + lista. Si P2 no tiene esa lista, suena como antes (banco de P1).
- **Límites:**
  - los proyectiles y explosiones (lanzagranadas, ballesta…) suenan desde el update de efectos, fuera del contexto de P2, y siguen usando el banco de P1;
  - P1 y P2 comparten los slots 8 y 9 (alternos), así que tres disparos seguidos cortan el más antiguo, como con un solo jugador;
  - el casquillo (`CallYakkyouSe`) sale de un efecto y usa el banco de P1.
- **Medido** (partida de prueba, P2 con el cuchillo, `ARMS_012`):
  - `coop_se_state = 2`, 6 muestras, 45.632 B;
  - el HD fusionado (1.664 B) está en el buffer del puerto 7 del IOP y `se_max[4] = 63`;
  - las 6 muestras están byte a byte en la SPU2 desde 0x1E7400 y la voz de Claire sigue intacta en 0x1D9C00;
  - al dar P2 una cuchillada, dos voces de la SPU2 tienen su dirección de inicio en las muestras 8 y 9 del banco fusionado (0x1EF440 y 0x1F0A20).
- **Sin `COOP`,** `sdfunc.o` sale idéntico (secciones de código y datos).

### Checklist de prueba manual

| # | Prueba | Estado |
| --- | --- | --- |
| 1 | P1 con el mechero o la pistola y P2 con el cuchillo: se oyen las cuchilladas de P2 | Pendiente (verificado en la RAM, falta oírlo) |
| 2 | P1 con el cuchillo y P2 con la pistola (`COOP_TEST`): cada uno suena con su arma | Pendiente |
| 3 | P2 cambia de arma desde su inventario: suena la nueva | Pendiente |
| 4 | La voz de Claire (daño, muerte) sigue sonando bien para los dos | Pendiente |
| 5 | Cambio de personaje a Chris y vuelta: los sonidos de P2 siguen bien | Pendiente |
| 6 | Salas con mucha reverb y vídeos: sin ruidos en los sonidos de P2 | Pendiente |

## Hito 7: P2 de cualquier personaje y mercenarios a dos

Spec: [2026-10-10-coop-hito7-design.md](../superpowers/specs/2026-10-10-coop-hito7-design.md). Plan: [2026-10-10-coop-hito7.md](../superpowers/plans/2026-10-10-coop-hito7.md).

### Implementación (7a)

- **Quién es P2:** `coop_p2_id` (0 Claire, 1 Chris, 2 Steve, 3 Wesker) y `coop_p2_cos` (traje, solo Claire). En la historia, Claire con el traje que no lleva P1 (como antes); en mercenarios, lo que elige P2. `coop_p2_ld_id` es el personaje cargado.
- **Identidad en los contextos de P2** (`coopSwapId`/`coopRestoreId`, una copia por contexto: update de P2, enemigo que va a por P2, pantalla de P2): `sys->ply_id`, `sys->costume` y `sys->plzmtp` (datos z de `bhGetTransZ`) son los de P2. En la pantalla de P2 también se guarda y repone la mochila (`gm_flg 0x8000000`), que `StatusMain` calcula con el personaje.
- **Carga:** `coopReadPlayer2Data` copia las animaciones de cuerpo a `coop_bmt2` (260 KB, reservado en `coopInitMemory`; el máximo medido es Wesker, 256.020 B) y las realiza en `coop_mnw2[0..99]`; los datos z van a `sys->lmmdlp + 0x5000` (12 KB libres tras la coleta). El fichero es `SYSTEM.AFS[id + 10 + traje*4]` y el arma `SYSTEM.AFS[20 + id*30 + arma]`. `PlyInfo[id]` da el radio y la altura.
- **Voz** (coopsnd.c, G20, G23 y G28): en cada carga de P2 se pide su `CORE_<id>.SPQ` (flag 6). Sus 4 programas se añaden al HD del banco 4 en las listas 4-7, tras una entrada falsa que corta la duración de la última muestra de armas; sus muestras van al hueco bajo de la SPU2 (0x050A0, desplazamientos `0xFFE2B4A0 + off`, que el IOP suma en 32 bits). G23 remapea solo si el personaje de P2 no es el de P1. **Riesgo:** la subida del BD escribe en 0x2050A0 y depende de que la SPU2 dé la vuelta a los 2 MB; PCSX2 lo hace, el hardware real está sin comprobar. Ver [world-systems.md](../architecture/world-systems.md#driver-de-sonido-iop-y-memoria-spu2).
- **Límites de la voz:** el slot 7 es compartido (un grito de P2 corta uno de P1) y suena en la posición de P1; la voz de muerte del game over (`CallPlayerDeadVoice`) es la del personaje de P1.
- **Objeto enganchado** (`coopSetLinkObj`): la coleta de Claire (`lkmtab[0]`, simulada) o las gafas de Wesker (`lkmtab[1]`, rígidas en la cabeza: solo posición y `bhCalcModel`). Chris y Steve no llevan.
- **G22:** en mercenarios, P2 no se carga en la carga completa (es la sala de selección), sino en el paso 10 del modo 4 de la primera sala tras elegir; después se vuelve a llamar a `coopRoomStart` (que `bhFinishRoom` ya había llamado sin P2). Solo carga si `coop_loaded == 0`: el personaje de P2 no cambia dentro de una partida de mercenarios (morir o terminar pasa por una carga completa).

### Implementación (7b)

- **Fases** (`coop_sel`: 0 elige P1, 1 elige P2, 2 hecho; se pone a 0 en cada carga completa, así que tras morir se empieza por P1):
  1. P1 elige en la pantalla original (traje, personaje e inventario como siempre).
  2. G24: la puerta 5-50 → 5-52 se cambia a 5-50. Se guardan el traje de P1 y la vista subjetiva (`gm_flg & 0x10028C0` y `ev_flg` 317), y empieza la fase P2. El cargador aplica el cambio de personaje de P1 al volver a entrar.
  3. En la fase P2 (`coopSelP2`), `coopSetPad2` pone el mando 2 en `sys->pad_*` (con la máscara de `bhSysCallPad`) y el mando 1 no cuenta. G25, G26 y G27 desvían el traje, el personaje y el inventario a P2.
  4. G24 deja pasar la segunda salida y repone el traje y la vista de P1. G22 carga a P2 en `RM_5520`.
- **Sin mando 2** conectado al confirmar P1, no hay fase P2: P1 juega solo. Si la carga de P2 falla en la primera sala, no se reintenta hasta la siguiente carga completa.
- **Mando:** el historial de P1 (`pad_oncpy`) se guarda al empezar la fase P2 y se repone al salir; la vuelta a 5-50 usa la posición por la que se entró.
- **Inventario de P2** (`coopBattleItemInit`): copia de la tabla de `ExtraGameItemInit`, con el Linear Launcher delante si está desbloqueado, la vida al máximo y sin veneno.
- **Pantalla de estado de P2:** su retrato sale del cambio de `ply_id` (7a); el mapa sigue bloqueado y la mochila activa como en el original.

### Checklist de prueba manual

| # | Prueba | Estado |
| --- | --- | --- |
| 1 | Historia: P2 sigue siendo Claire B (o A), con su coleta; se oculta en la parte de Chris y vuelve con Claire | Pendiente |
| 2 | Historia con `COOP_TEST` y `COOP_TEST_P2_ID` 1, 2 o 3: P2 es Chris, Steve o Wesker (gafas), anda, corre, apunta y dispara con sus animaciones | Pendiente |
| 3 | El inventario de P2 muestra su retrato y su mochila; al cerrarlo, P1 conserva la suya | Pendiente |
| 4 | P2 recibe daño y grita con su voz; P1 con la suya | Pendiente |
| 5 | Un zombi agarra a P2 (Chris o Steve): la animación del agarre encaja | Pendiente |
| 6 | Mercenarios: P1 elige con el mando 1; la pantalla se repite y P2 elige con el mando 2 (el mando 1 no responde) | Pendiente |
| 7 | Cada uno aparece en la primera sala con su personaje y su inventario de mercenarios, también repitiendo personaje | Pendiente |
| 8 | Con el Linear Launcher desbloqueado, los dos lo tienen | Pendiente |
| 9 | La vista subjetiva que elige P2 no cambia la de P1 | Pendiente |
| 10 | Muere P2 → game over → vuelta a la selección empezando por P1 | Pendiente |
| 11 | Llegar al final con los dos: el ranking es el de P1 | Pendiente |
| 12 | Combinaciones grandes (P1 Claire + P2 Wesker, Claire B + Claire) cargan en todas las salas de mercenarios | Pendiente |
