# Mapa de archivos

Qué hace cada archivo del juego, agrupado por subsistema. Las rutas son relativas a `src/ps2/veronica/prog/` salvo que se indique otra cosa. Entre paréntesis, el número aproximado de líneas cuando el archivo es grande.

Para ver todas las funciones de un archivo: `grep -n "^[A-Za-z_].*(" archivo.c | grep -v ";"`.

## Arranque, sistema y bucle principal

| Archivo | Qué hace |
| --- | --- |
| `njloop.c` | `main()`: llama a `njUserInit` y luego a `njUserMain` + `njWaitVSync` en bucle. |
| `main.c` | Globales del motor (`sys`, `rom`, `ply`, `plp`, `ene[128]`, `eff[512]`, `cam`, `pd_port`…), la tabla de tareas `bhSysTaskJumpTab[23]`, `njUserInit` (memoria, vídeo, Ninja) y `njUserMain` (ejecuta las tareas activas). |
| `system.c` (2.5k) | Las tareas `bhSysCall*`: inicio, título, mando, juego, eventos, menús, puertas, cargador (`bhSysCallMonitor`). También `bhInitSystem`, `bhInitRoomChangeSystem` y `bhFirstGameStart` (partida nueva). |
| `game.c` | `bhMainSequence`, que es un frame de juego (enemigos, jugador, efectos, objetos, cámara, luz, eventos), y `bhAllDrawModel`, el orden de dibujo. |
| `sync.c` | VSync, la función de fin de frame `bhControlEOR`, la comprobación de conexión del mando (`bhCheckPadPort`) y el soft reset. |
| `screen.c` | Fundidos, bandas de cine (cinesco), mira telescópica, termómetro, salvapantallas y texturas de render a pantalla. |
| `sbinit.c` | Inicialización y cierre del sistema Katana. |
| `ps2_event.c` | Color de fondo durante eventos. |

## Jugador

| Archivo | Qué hace |
| --- | --- |
| `player.c` (7.3k) | El jugador: init (`bhInitPlayer`, `bhSetPlayer`), update (`bhControlPlayer`), máquina de estados `bhCPM0_*` / `bhCPM1_*` / `bhCPM2_act_*` (andar, correr, escaleras, empujar, apuntar, disparar, recargar, cuchillo, daño, muerte). |
| `playpch.c` | Apuntado del torso y brazos (IK), fijación de blanco (`bhSearchEnemy`, `SetLockOnDirection`) e impacto de disparo (`CheckGunHit`). Variantes `_pch` de los estados de ataque. |
| `playpch2.c` | Apuntado con mira telescópica (`bhCPM2_act_scp`). |
| `pl_evt.c` | El jugador controlado por cinemáticas: `bhCPM0_event` (mode0 = 7) y los movimientos de guion `pl_smove00..08`. |
| `weapon.c` | Objetos de arma (`bhSetWeapon`), munición (`bhCheckBullet`, `bhCountBullet`) y detección de impactos de pistola, cuchillo, arco y explosivos. |
| `dread.c` | Carga los datos del personaje (`bhReadPlayerData`) y del arma (`bhReadWeaponData`) en los buffers fijos del jugador. |
| `pwksub.c` (3.5k) | Ayudas genéricas para cualquier `BH_PWORK`: asignador de memoria `bhGetFreeMemory`, número de piso, velocidad, buscar el enemigo o el jugador más cercano, recorte de visibilidad (`bhCheckClipModel`), salpicaduras, fogonazos, casquillos, llama del mechero. |

## Enemigos y NPCs

Ver [enemies-and-npcs.md](enemies-and-npcs.md) para el catálogo de qué criatura es cada `enNN.c`.

| Archivo | Qué hace |
| --- | --- |
| `eneset.c` | `ene[]`: tabla de despacho por id `bhJumpEnemy[100]`, alta de entidades (`bhSetEnemy`), update (`bhControlEnemy`), dibujo (`bhDrawEnemy`) y callbacks instalables (`bhEne_SetCallFunc`). |
| `en01.c` … `en30.c`, `en54.c`, `en55.c`, `en71.c` | Un tipo de enemigo o entidad por archivo (`bhEneNN`). Patrón habitual: `_Init`, `_Brain`/`_BR*` (decidir), `_Move`/`_MV*` (moverse), `_Nage`/`_NG*` (agarrar al jugador), `_Damage`/`_DG*` (recibir daño), `_Die`/`_DD*` (morir), `_PlayerControl` (mover al jugador durante un agarre). |
| `en01b.c`, `en01sub.c`, `en02sub.c`, `en03sub.c`, `en05sub.c`, `en06sub.c`, `en13sub.c`, `en17sub.c` | Partes y variantes de algunos enemigos: brazos, piernas y cabeza separables, crías, hijos. |
| `zonzon.c`, `zonzon1.c` | Ayudas compartidas por la IA de enemigos: dirección al blanco, paredes, sangre y efectos, cálculo de daño, sonido, palancas. |
| `subpl.c` | NPCs controlados por guion (Steve, Rodrigo, Alfred…): ids 31-99 que despachan a `bhSubpl`. No tienen IA ni colisión. |
| `face.c`, `face_bh.c` | Animación facial y sincronización labial de los NPCs con cara (ids > 90). |

## Sala, mundo y colisión

| Archivo | Qué hace |
| --- | --- |
| `room.c` | Carga de sala desde el RDT (`bhInitReadRDT`, `bhSetRoom`, `bhFinishRoom`), modelos y animaciones de enemigos de la sala, objetos e items, inicio de puerta (`bhSetDoorDemo`, `bhStartDoorDemo`) y la instantánea para reintentar (`bhPushGameData`, `bhPopGameData`). |
| `door.c` | La animación de transición de puerta (la escena de abrir puerta mientras carga la sala siguiente). No toca al jugador. |
| `cut.c` (2.9k) | Cámaras: cambio de plano fijo según la posición del jugador (`bhCheckCut`, `bhSetCut`), cámaras que siguen (`ActiveCamera`), cámaras de evento y primera persona (`PlEyeCamera`). |
| `camera.c` | Construye la matriz de cámara a partir de `cam`. |
| `hitchk.c` (5.9k) | Colisión con la sala: paredes (`bhCheckWall`, `bhCheckWallEx`), suelo, escalones y escaleras (`dansa`, `kaidan`), el botón de acción para puertas, items y examinar (`bhCheckExmAtari`), agua, y empuje entre personajes (`bhCheckPlayer`, `bhCheckEnemies`). |
| `hitchkl.c` | Colisión de líneas y segmentos contra planos y modelos (línea de visión, balas). |
| `rutchk.c` | Comprobación de ruta entre dos puntos (`bhCheckRoute`). |
| `objitm.c` | Objetos de sala e items: update y dibujo, cajas que se empujan, objetos especiales (`bhObj001..012`), pelo y accesorios (`bhObjClpn`). |
| `light.c` | Luces de la sala por plano de cámara (`bhControlLight`, `bhSetLight`, `bhSetHalfLight`). |
| `flag.c` | Ayudas de bits: `bhStFlg`, `bhCrFlg`, `bhCkFlg`. |
| `map.c` (3k) | Pantalla de mapa del juego. No es colisión. |

## Eventos, menús y pantallas

| Archivo | Qué hace |
| --- | --- |
| `event.c` (13.7k) | Intérprete de los scripts de evento de cada sala (opcodes `bh*`), control de jugador y NPCs desde guion (`Player_controll`, `Sub_controll`, `bhLoadWork`). Ver [events-and-flags.md](events-and-flags.md). |
| `sp_evt.c` | Minijuego del terminal de ordenador (pantalla y teclado). |
| `message.c` | Mensajes en pantalla y fuentes. |
| `sub1.c` (10k) | Inventario y pantalla de estado: items, combinar, usar (curar), armas, ECG de salud. |
| `item.c` | Tablas de datos de items. No tiene funciones. |
| `itemview.c` | Examinar un item en 3D. |
| `fileview.c` | Lectura de archivos (documentos). |
| `bup_00.c` | La máquina de escribir (guardar partida). |
| `gameover.c`, `ranking.c` | Game over / continuar, y ranking final. |
| `adv.c` (3.7k) | Título, menús, opciones, fundidos de menú, salvapantallas. Lee la estructura `Pad[]` directamente. |
| `ps2_SaveScreen.c`, `ps2_LoadScreen.c`, `ps2_SystemSaveScreen.c`, `ps2_SystemLoadScreen.c` | Pantallas de guardar y cargar en tarjeta de memoria. |
| `ps2_McSaveFile.c`, `ps2_MemoryCard..c`, `backup.c`, `ps2_sg_bup.c` | Lectura y escritura en tarjeta de memoria. La partida guardada es una copia de `SYS_WORK` desde `version` hasta `save_end`. |

## Modelos, animación y render

| Archivo | Qué hace |
| --- | --- |
| `MdlPut.c` | Dibujar un `BH_PWORK` (`bhPutModel`) y calcular sus matrices de huesos (`bhCalcModel`, `bhCalcTree`). |
| `Motion.c` | Reproducir y mezclar animaciones (`bhSetMotion`, `SetMtn*`, variantes `Hokan` = interpolación). |
| `binfunc.c` | Reubicar en memoria los binarios de modelo y animación (`bhMlbBinRealize`…). No es idempotente: no hay que llamarlo dos veces sobre los mismos datos. |
| `njplus.c` | Extensiones del Ninja: skinning (`npSkinConvert`, `npCalcSkin`), colisión de cápsulas y esferas, ayudas de memoria. |
| `effect.c` + `effsub0.c` … `effsub6.c` (~30k en total) | Sistema de efectos `eff[512]`: alta (`bhSetEffect`), sombras (`bhSetShadow`), update y dibujo por tipo. `bhEffNNN` es el efecto número NNN; `bhEff_ENN_*` son efectos propios del enemigo NN. |
| `ps2_Na*.c`, `ps2_Ninja*.c` | La API Ninja/Katana (de Dreamcast) reimplementada para PS2: matrices, matemáticas, colisión, texturas, vista, dibujo 2D y 3D, modelos chunk (`ps2_NinjaCnk.c`). |
| `ps2_Vu1Strip.c`, `ps2_Vu1Scissor2.c` | Rasterizado de tiras de triángulos en la VU1 y recorte. |
| `ps2_dummy.c` (2.6k) | Inicialización de PS2 (módulos IOP, mando, CD, tarjeta), tablas de orden (OT), sombras y `PS2_jikken` (fin de frame: intercambio de buffers). |
| `ps2_texture.c`, `ps2_loadtim2.c`, `ps2_pxlconv.c` | Gestión de textura principal y VRAM, carga de TIM2 y conversión de píxeles. |

## Entrada

Ver [input.md](input.md).

| Archivo | Qué hace |
| --- | --- |
| `pad.c` | `bhSetPad`: convierte el mando físico en los campos lógicos `sys->pad_*` (aplica la configuración de botones). |
| `ps2_sg_pad.c` | Capa PS2 del mando: `Pad_init`, `Ps2_pad_read` (solo puerto 0), `pdGetPeripheral` (con caché por frame). |
| `ps2_Ninjapad.c` | `njGetPeripheral`, un envoltorio. |
| `padman.c` | La estructura `Pad[4]` al estilo Dreamcast (la usan los menús), repetición de teclas y soft reset. |
| `ps2_sg_pdvib.c`, `vibman.c` | Vibración (solo puerto 0). |

## Sonido, vídeo y ficheros

| Archivo | Qué hace |
| --- | --- |
| `sdfunc.c` (3.5k) | Sonido de alto nivel (efectos del jugador, enemigos, puertas, BGM, sonido 3D), arranque de Sofdec, montaje de AFS. |
| `sdc.c`, `sdcwrap.c`, `ps2_sg_sd.c`, `ps2_snddrv.c` | Driver de sonido (MIDI y efectos) y comunicación con el IOP. |
| `adxwrap.c`, `ps2_cri_adxt.c` | Streaming de ADX (voces y música) y lectura de ficheros dentro de AFS. |
| `mwwrap.c`, `ps2_sfd_mw.c`, `ps2_MovieFunc.c`, `ps2_MovieWork.c` | Vídeo (Sofdec/MPEG). |
| `gdlib.c`, `ps2_sg_gd.c`, `ps2_dvd_image.c` | Sistema de ficheros del disco. |
| `expand.c` | Descompresión de datos (por ejemplo las salas `.rdx`). |
| `ps2_sg_maloc.c` | `syMalloc` / `syFree` sobre el arena estático `Ps2_malloc_mem` (12.8 MB). |
| `ps2_sg_sy*.c`, `gcc_wrapper.c` | Adaptaciones menores de Katana (reloj, temporizador, configuración) y comparaciones de `double`. |

## Fuera de `prog/`

| Ruta | Qué es |
| --- | --- |
| `src/cri/`, `src/users/oshimi/` | Librerías CRI ADX y sj (middleware de audio). Se compilan con gcc. |
| `src/sce/ee/lib/crt0.s` | Arranque del ejecutable. |
| `src/JPN/` | Archivos solo de la versión japonesa (SLPM_650.22). |
| `src/*.c` (`atick.c`, `tq.c`, `tsnddrv.c`…) | Esqueletos con direcciones del driver de sonido IOP `TSNDDRV.IRX`. No se compilan. |
| `include/ps2/veronica/prog/` | Cabeceras del juego. `types.h` contiene todos los structs, con el offset de cada campo. |
| `include/recvx-decomp-*` | Submódulos (compilador MWCC, cabeceras Katana y CRI) y el SDK de PS2, que no se incluye en el repo. |
| `config/` | Configuración de splat (`SLUS_201.84.yaml`), linker script (`.lcf`) y direcciones originales de símbolos (`symbol_addrs.txt`). |
