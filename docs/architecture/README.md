# Arquitectura del motor (RE: Code Veronica X, PS2)

Mapa para moverse por el código del juego. Todo está comprobado contra el código salvo lo marcado como "probable", "deducido" o "sin confirmar". Las rutas son relativas a `src/ps2/veronica/prog/` salvo que se indique otra cosa. Los números de línea de este README, de [combat.md](combat.md) y de [inventory.md](inventory.md) son del árbol de trabajo con los ganchos del cooperativo (octubre de 2026). En los demás documentos pueden estar unas 4-8 líneas por encima en los archivos con ganchos (player.c, system.c, room.c, dread.c, game.c). Si no cuadran, busca por el nombre de la función.

## Documentos

| Documento | Contenido |
| --- | --- |
| [file-map.md](file-map.md) | Qué hace cada archivo `.c`, agrupado por subsistema |
| [game-loop.md](game-loop.md) | Arranque, tabla de tareas, orden de un frame y orden de dibujo |
| [player.md](player.md) | `BH_PWORK`, el jugador (`ply`/`plp`), ciclo de vida, máquina de estados, carga de modelo y animaciones, qué se puede compartir entre instancias |
| [input.md](input.md) | Del mando físico a `sys->pad_*`, bits lógicos y quién lee la entrada |
| [rooms-and-memory.md](rooms-and-memory.md) | Memoria, `SYS_WORK` y guardado, `ROM_WORK`, el cargador, la carga de sala y las puertas |
| [world-systems.md](world-systems.md) | Cámaras, colisión, objetos, luces, sombras, dibujo y sonido, con su dependencia de un único jugador |
| [combat.md](combat.md) | Apuntar, disparar, munición, objetos de arma (`obwp`), `WPN_TAB` y daño a enemigos |
| [inventory.md](inventory.md) | `sys->itm`, inventario y pantalla de estado, baúles, máquina de escribir, equipar armas |
| [enemies-and-npcs.md](enemies-and-npcs.md) | `ene[128]`, alta de entidades, catálogo de enemigos y NPCs por guion (`subpl`) |
| [events-and-flags.md](events-and-flags.md) | Scripts de evento y glosario de flags de `SYS_WORK` / `BH_PWORK` |

El proyecto cooperativo tiene su propio documento: [../coop/README.md](../coop/README.md).

## Globales principales

| Global | Tipo | Definido en | Qué es |
| --- | --- | --- | --- |
| `sys` | `SYS_WORK*` | main.c:57 | Estado global: flags, mando, memoria, punteros a datos del jugador, partida guardada |
| `rom` | `ROM_WORK*` | main.c:58 | Tablas de la sala cargada |
| `ply` / `plp` | `BH_PWORK` / `BH_PWORK*` | main.c:40 / :59 | El jugador y el puntero que usa todo el código |
| `ene[128]` | `BH_PWORK[]` | main.c:41 | Enemigos y NPCs de la sala |
| `eff[512]` | `O_WRK[]` | main.c:42 | Efectos (incluidas las sombras) |
| `cam` | `CAM_WORK` | main.c:39 | La cámara |
| `hws` | `HWS_WORK*` | main.c:56 | Modo de vídeo |
| `pd_port` | `int` | main.c:55 | Puerto del mando activo (0) o -1 si está desconectado |

## Búsqueda rápida

| Busco… | Función | Dónde |
| --- | --- | --- |
| Tabla de tareas / bucle | `bhSysTaskJumpTab`, `njUserMain` | main.c:60, :189 |
| Un frame de juego | `bhMainSequence` | game.c:24 |
| Orden de dibujo | `bhAllDrawModel` | game.c:325 |
| Partida nueva | `bhFirstGameStart` | system.c:420 |
| Cargador (partida, sala, personaje) | `bhSysCallMonitor` | system.c:1313 |
| Carga de sala | `bhInitReadRDT`, `bhSetRoom`, `bhFinishRoom` | room.c:32, :51, :341 |
| Instantánea para reintentar | `bhPushGameData`, `bhPopGameData` | room.c:1024, :1045 |
| Puertas | `bhSetDoorDemo`, `bhStartDoorDemo` | room.c:891, :948 |
| Init del jugador | `bhInitPlayer`, `bhSetPlayer`, `bhInitRoomChangePlayer` | player.c:639, :697, :863 |
| Update del jugador | `bhControlPlayer` | player.c:1357 |
| Estados del jugador | `bhCtrPly_mode0[]`, `bhCPM0_action`, `bhCPM1_act_bas`, `bhCPM1_act_atk` | player.c:1341, :1785, :1965, :4489 |
| Movimiento según el mando | `bhControlPlayerPad` | player.c:1828 |
| Muerte del jugador | `bhCPM0_die` | player.c:6354 |
| Jugador en cinemática | `bhCPM0_event` | pl_evt.c:62 |
| Carga de modelo y arma | `bhReadPlayerData`, `bhReadWeaponData` | dread.c:17, :178 |
| Apuntar y disparar | `PlyPchMain`, `CheckGunHit`, `bhCheckGunAtari` | playpch.c:18, :1130; weapon.c:473 |
| Objetos de arma | `bhSetWeapon` | weapon.c:128 |
| Objetos de arma y pelo en la sala | `bhSetObject`, `bhObjClpn` | objitm.c:154, :2122 |
| Munición | `bhCheckBullet`, `bhCountBullet`; `bhSearchBullet` | weapon.c:421, :439; sub1.c:8862 |
| Equipar un arma | `WeaponSet` | sub1.c:4456 |
| Objetos iniciales de cada personaje | `AllItemInit` | sub1.c:8440 |
| Abrir y cerrar el inventario | `ItemTaskCheck` (la petición sale de `bhCheckSubTask`) | sub1.c:3025; system.c:568 |
| Mando lógico | `bhSetPad` | pad.c:17 |
| Mando físico | `Pad_init`, `Ps2_pad_read`, `pdGetPeripheral` | ps2_sg_pad.c:646, :194, :74 |
| Enemigos | `bhControlEnemy`, `bhSetEnemy`, `bhDrawEnemy`, `bhJumpEnemy[]` | eneset.c:344, :170, :797, :51 |
| Callback de entidad | `bhEne_SetCallFunc`, `bhEne_CallocWork` | eneset.c:933, :906 |
| NPC de guion | `bhSubpl` | subpl.c:76 |
| Cámara fija | `bhCheckCut`, `bhSetCut` | cut.c:12, :281 |
| Paredes / suelo | `bhCheckWallEx`, `bhCheckFloorP`, `bhGetGroundPosition` | hitchk.c:641, :5245, :3557 |
| Botón de acción | `bhCheckExmAtari` | hitchk.c:4588 |
| Empuje entre personajes | `bhCheckPlayer`, `bhCheckEnemies` | hitchk.c:5712, :5788 |
| Dibujar / matrices | `bhPutModel`, `bhCalcModel` | MdlPut.c:17, :240 |
| Animación | `bhSetMotion` | Motion.c:21 |
| Sombra / efecto | `bhSetShadow`, `bhSetEffect` | effect.c:644, :331 |
| Memoria lineal / malloc | `bhGetFreeMemory`, `syMalloc` | pwksub.c:16; ps2_sg_maloc.c:32 |
| Scripts de evento | `bhInitEvent`, `bhControlEvent` | event.c:339, :379 |
| Guion moviendo al jugador o a NPCs | `bhLoadWork`, `Player_controll`, `Sub_controll` | event.c:12713, :9271, :9758 |
| Cambio de personaje | `bhPlayerChangeSet` | event.c:6264 |
| Inventario / estado | `StatusInit`, `StatusMain` | sub1.c:1389, :3242 |

## Convenciones de nombres

**Prefijos**

| Prefijo | Significado |
| --- | --- |
| `bh` | Biohazard: código del juego |
| `nj` | Ninja, la librería 3D de Katana (Dreamcast), reimplementada para PS2 en `ps2_Na*.c` / `ps2_Ninja*.c` |
| `np` | Extensiones del Ninja (njplus.c) |
| `sy`, `pd`, `gd`, `bu`, `sd` | Librerías Shinobi de Katana: sistema, periféricos (mando), GD-ROM (ficheros), backup (tarjeta), sonido |
| `Ps2` | Capa de adaptación a PS2 |
| `sce` | SDK de Sony |
| `ADX`, `mw`, `sfd` | Middleware CRI (audio y vídeo) |

**Sufijos de funciones**

| Sufijo | Significado |
| --- | --- |
| `CPM0` / `CPM1` / `CPM2` | Niveles de la máquina de estados del jugador (`mode0`/`mode1`/`mode2`) |
| `_Brain` / `_BRnn` | Decisión de la IA |
| `_Move` / `_MVnn` | Movimiento |
| `_Nage` / `_NGnn` | Agarre al jugador |
| `_Damage` / `_DGnn` | Recibir daño |
| `_Die` / `_DDnn` | Morir |

**Términos en japonés (romaji)**

| Término | Significado |
| --- | --- |
| `atari` | Colisión o impacto |
| `nage` | Agarrar o lanzar |
| `kaidan` | Escaleras |
| `dansa` | Escalón |
| `hokan` | Interpolación |
| `yakkyou` | Casquillo de bala |
| `rinpun` | Polvo de escamas (de la polilla) |
| `nikuhen` | Trozos de carne |
| `kega` | Herida |
| `muteki` | Invencible |
| `san` | Ácido (probable) |
| `jikken` | Experimento |

**Estilo del código decompilado**

- Los comentarios `// 100% matching!` o `// 99.96% matching` indican cuánto se parece una función al binario original.
- Los flags se escriben como literales hex sin nombre.
- Los campos se escriben de golpe con `*(int*)&p->mode0 = …`.

## Recetas de búsqueda

- **Definición de una función.** Las definiciones empiezan en la columna 0 y los prototipos acaban en `;`:
  `grep -n "^[A-Za-z_].*[ *]NOMBRE(" src/ps2/veronica/prog/*.c | grep -v ";"`
- **Todas las funciones de un archivo:**
  `grep -n "^[A-Za-z_].*(" archivo.c | grep -v ";"`
- **Structs y offsets.** Están en `include/ps2/veronica/prog/types.h`, con el offset de cada campo en un comentario. `EXP_WORK` y `WPN_TAB` están en `player.h`.
- **Macros de acceso a `exp0`/`exp1`** (`PEXP0_F(off)`, `EXP1_I(off)`, `EXP0_F(off)` para `epw`…): `include/ps2/veronica/prog/macros.h`.
- **Dirección original en el ejecutable retail:** `config/symbol_addrs.txt`.
- **Quién usa un campo:** `grep -n "sys->campo"`, o `grep -n "plp->campo"` para el jugador.
