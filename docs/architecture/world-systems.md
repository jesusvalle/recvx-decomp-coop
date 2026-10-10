# Sistemas del mundo: cámara, colisión, objetos, luces y sonido

Rutas relativas a `src/ps2/veronica/prog/`. Cada sección indica si el sistema recibe la entidad como **parámetro** o usa el **global `plp`**. Esto es lo que decide si un sistema sirve tal cual para un segundo personaje.

## Cámara

Hay una sola cámara global, `CAM_WORK cam` (types.h:1062, main.c:39). No hay pantalla partida, pero el motor sabe dibujar la escena una segunda vez desde otro plano (ver [Dibujo](#dibujo)).

### Planos fijos

- `bhCheckCut` (cut.c:12) se llama en cada frame desde `bhMainSequence`. Comprueba `plp->gpx/gpz/flr_no` contra las zonas de `rom->cutp[]` con `bhCheckCutArea` (cut.c:155, que **recibe parámetros**). Para cambiar de plano, los 4 puntos de prueba (±1.0) tienen que haber salido del plano actual (cut.c:112-117). Entonces llama a `bhSetCut` (cut.c:281).
- `bhSetCut` también oculta mallas y luces de la sala según el plano (`hidobj`/`hidlgt`). La geometría que no se ve desde el plano actual puede no dibujarse.
- `bhControlCamera` (camera.c:23) solo construye la matriz a partir de `cam`.

### Cámaras que siguen

- `bhInitActiveCamera` (cut.c:663) y `bhControlActiveCamera` (cut.c:1174) siguen a `cam.plx/ply/plz`, que salen de `plp->exp0` (EXP_WORK `plx/ply/plz`).
- **Para una cámara cooperativa, este es el punto de enganche natural** (por ejemplo, apuntar al punto medio entre los dos jugadores).

### Cámaras de evento

- Los fotogramas clave llevan un enlace `lkflg/lkno/lkono`. `bhGetEvtCamLockPosition` (cut.c:2391) lo resuelve: 1 = `plp`, 2 = `ene[lkno]`, 3 = `obwp`, 4 = `itwp`, 5 = `eff`, 6 = `rom->posp`.
- Las luces usan el mismo patrón de enlace (light.c:720-743).

### Primera persona

- `bhInitPlEyeCamera`, `bhControlPlEyeCamera` y `bhSetPlEyeCamera` (cut.c:2472-2800) usan el hueso de la cabeza de `plp` (`owP[5]`).

Si un personaje que no es `plp` sale del encuadre, no pasa nada: se sigue actualizando y `bhCheckClipModel` (pwksub.c:2465) lo descarta al dibujar.

## Colisión con la sala

| Función | Línea | ¿Parámetro o `plp`? | Notas |
| --- | --- | --- | --- |
| `bhCheckWallEx(pw, npos, opos, r, h)` | hitchk.c:641 | Parámetro | Paredes de `rom->walp` + `sys->mwalp`. Si no choca, ajusta `npos->y` a la altura del piso. Excepciones: el atributo 0x10000 solo se trata si `pw == plp` (:722); las paredes que hacen daño (atributo 0x40) dañan a **`plp`** (:967-1001). El jugador la llama dos veces por frame (cuerpo player.c:1622, pies :1647). |
| `bhCheckWall(pw)` | hitchk.c:12 | Parámetro | La usan los enemigos. Mismo problema de daño a `plp` (:259-287). |
| `bhSetFloorNum(pw)` | pwksub.c:85 | Parámetro | Calcula el piso. |
| `bhGetGroundPosition(p)` | hitchk.c:3557 | Parámetro | Altura del suelo; escribe el global `sys->htp`. |
| `bhCheckFloorP(pp)` | hitchk.c:5245 | **Mixto** | Lee `plp->exp0` y `plp->flg`, borra y activa los activadores globales `sys->cb_flg` (0x200) / `flr_idx`. Los resultados por personaje (sonido de pisada, agua, escalón) van a `pp`. |
| `bhCheckExmAtari(pp)` | hitchk.c:4588 | **`plp`** | Botón de acción. Sondea desde `plp`. Tipos de zona: 0 = puerta (rellena `sys->door` y activa el bit 0x1 de `cb_flg`), 1/2 = escalera o escalón (`sys->pl_htp`), 3 = examinar o cámara fija, 4 = coger item u objeto. |
| `bhCheckEnemies(pp)` | hitchk.c:5788 | Parámetro | Separa a `pp` de los enemigos. Tiene una rama `pp == plp` (:5844). |
| `bhCheckPlayer(pp)` | hitchk.c:5712 | **`plp`** | Lo llaman los enemigos para no atravesar al jugador; solo considera a `plp`. |
| `bhCollisionCheckLine*` | hitchkl.c | Parámetro | Líneas de visión y balas. |
| `bhCheckRoute` | rutchk.c:5 | Parámetro | |

Las escaleras (`kaidan`) y escalones (`dansa`) leen el global `sys->pl_htp` (player.c:2882, 3144, 3939, 4134).

## Objetos e items ([objitm.c](../../src/ps2/veronica/prog/objitm.c))

- `sys->obwp[32]` son los objetos (`O_WRK`, 0x4E0 B cada uno; reservados en system.c:450): los índices 0-1 son las armas (y manos) del jugador, el 2 su pelo o accesorio (gafas de Wesker), el 3 no se usa (solo se vacía en el cambio de personaje, system.c:1752), y los objetos de la sala empiezan en el 4. `sys->itwp` son los items.
- **Solo se actualizan y dibujan los índices `< rom->obj_n`** (objitm.c:297-306). `obj_n` va de 4 a 29 en las 205 salas, así que 29-31 nunca se usan, pero tampoco se actualizarían.
- Actualización (`bhControlObjItm`, objitm.c:282-397, después de los jugadores): un objeto enlazado (`flg & 0x80`, `lkwkp` = dueño, `lkono` = hueso, `lox/loy/loz` = desplazamiento) hereda la ocultación del dueño y se coloca con `njCalcPoint(owner->mlwP->owP[lkono].mtx, …)`. Después se llama a `bhJumpObject2[id - 1200]` (objitm.c:125-140): 1210 `bhObjWpn` (arma), 1211 `bhObjClpn` (pelo), 1212 `bhObjWssg` (gafas). Los objetos con `id ≥ 1210` se añaden a la lista `ob_hlg` si `pt_flg & 1`.
- Dibujo: `bhDrawObjItm` → `bhDrawObject` → `bhPutModel(op)`, con luz media y `amb_chr` (objitm.c:518-529, 631-680). Las armas y el pelo los dibuja el sistema de objetos, no el dibujo del jugador.
- Enganchar un objeto a un dueño: `bhSetObject(tab, n, owner)` (objitm.c:154-215). El pelo usa `lkmtab[0] = {flg 129, id 1211, mdlver 4}` (player.c:26-30), con `lkono` 5 y `mdl[0]`/`skp[0]` = modelo 4 del dueño (player.c:764-793). Las armas, `bhSetWeapon` (ver [combat.md](combat.md)).
- Las cajas que se empujan (`bhObj001`) usan `plp->psh_idx` (objitm.c:1063-1138). Los objetos que giran solo empujan a `plp` (objitm.c:1673-1833).
- El pelo (`bhObjClpn`, objitm.c:2122-2383, con `bhCalcHair`, player.c:7215-7306) recibe el dueño como parámetro, pero su buffer de simulación sale de `sys->pletcp` solo si `op->lkwkp == plp`; si no, reserva unos 20 KB (0x4000 + 0x1000) por encima de `mempb`, que se pierden al cambiar de sala (objitm.c:2146-2155). `flg 0x100000` indica que el buffer ya está puesto. También escribe los ángulos de `op->mlwP->objP[]` (objitm.c:2363-2368).

## Luces y sombras

- La iluminación es **global y depende del plano de cámara**, no del personaje. `bhControlLight` (light.c:332) elige las luces de `rom->cutp[cam.ncut]`. `bhSetLight` y `bhSetHalfLight` se aplican antes de dibujar enemigos y jugador.
- Una luz con `lkflg == 1` sigue a `plp` (light.c:724-726): `bhControlLight` la coloca después del update de los jugadores. El fogonazo y las explosiones usan `rom->lgtp[0]`; el mechero, `rom->lgtp[1]` (player.c:1459-1495). Ver [combat.md](combat.md).
- **Sombras:** `bhSetShadow(jtb, owner, lkono, …)` (effect.c:644) enlaza un efecto de `eff[]` a cualquier dueño. El jugador la llama en player.c:791. Como `bhClearEffect` borra las sombras en cada carga de sala, hay que recrearlas.

## Dibujo

- `bhPutModel(pw)` (MdlPut.c:17), con la pasada de espejo incluida, y `bhCalcModel(pw)` (MdlPut.c:240) **reciben parámetro**.
- La llamada para el jugador está fija en `bhAllDrawModel` (game.c:357-370).
- **Ritmo:** `Ps2SwapDBuff` (ps2_NaSystem.c:67) espera al menos 2 vsync, así que el juego va a 30 fps fijos. La lógica avanza un paso por frame dibujado (`loop_ct` es siempre 1): si el dibujo tarda más de 2 vsync, el juego se ralentiza, no salta frames.
- **Lista de dibujo diferida:** `bhAllDrawModel` llena una OT que se envía en `Ps2DrawOTag` (y se vacía con `Ps2ClearOT`). Para cambiar algo que el GS aplica al momento (el recorte) entre dos partes de la escena, hay que enviar la OT antes.
- **Recorte y pantalla:** `njUserClipping(2, p)` (ps2_NaSystem.c:439) pone el scissor del GS en el rectángulo `p[0]..p[1]` y `njUserClipping(0, …)` lo restaura a pantalla completa. `njSetScreen` (ps2_NaView.c:27) fija la distancia de proyección, el tamaño y el centro (`cx/cy`) del área de dibujo. El inventario dibuja así el modelo 3D del objeto (sub1.c:3540-3580): `Ps2DrawOTag` → recorte → dibujo → `Ps2DrawOTag` → recorte completo.
- **Segunda pasada de la escena:** `bhDrawSmallScreenRenderTexture` (screen.c:920) dibuja la sala otra vez desde otro plano para los monitores (`gm_flg 0x200`): copia `cam`, `bhSetRenderCut` + `bhControlCamera`, `njSetScreen`, `bhAllEasyDrawModel` (sin jugador) y `Ps2DrawOTag`; después restaura `cam`, la proyección (`njSetScreenProjection`, `Ps2CalcScreenCone`), las mallas ocultas del plano (`bhSetHideObjLgt`) y las luces (`bhSetLight`). `bhDrawFullScreenRenderTexture` (screen.c:816) dibuja la escena entera a una textura de 512x480.
- **Lo que depende del plano al dibujar:** mallas ocultas de la sala (`evalflags 0x8`, globales en `rom->mdl.objP`), luces (`bhControlLight` lee `cam.ncut`), niebla (`cam.fog_*`), objetos ocultos por plano (`op->hide[]`, objitm.c:562, 659, 688) y el recorte de vista por sala y plano (`ViewClipTbl`, event.c:13534).

### Texturas

- **Pool:** `Ps2_tex_mem[10485760]` (ps2_dummy.c:35, 79), lineal, con 256 entradas `tbuf` (main.c:34, 160; `njInitTexture`, ps2_NaTextureFunction.c:60-144). Las texturas de render ocupan 1 MB cada una (ps2_NaTextureFunction.c:558-597).
- **Carga:** `bhSetMemPvpTexture(texP, datp, 0)` (ps2_texture.c:148-188) recorre los TIM2 de un bloque y llama a `njLoadTexture` (ps2_NaTextureFunction.c:204-336) para cada uno. **Se deduplica por índice global** (el `Gindex` del TIM2): si ya hay una textura con ese índice, se incrementa su contador y se reutiliza. Si no, `Ps2TextureMalloc` (:552-634) **copia** el TIM2 al pool, así que el buffer de lectura se puede tirar después.
- **Si el pool o las entradas `tbuf` se llenan, el juego hace `exit(0)`** (ps2_NaTextureFunction.c:286-289, 321-324). No hay `printf` del espacio libre: está en `Ps2_free_texmemsize` (ps2_NaTextureFunction.c:29).
- **Liberar:** `njReleaseTexture(texP)` decrementa el contador y libera al llegar a 0 (:401-455); después `bhGarbageTexture(NULL, 0)` compacta el pool (`Ps2TextureGarbageCollectionAll`, :719-769), como hace el cambio de personaje (system.c:1739-1747). `njReleaseTextureAll` lo reinicia todo.
- **Contador por índice global:** si dos dueños cargan texturas con el mismo `gindex`, al liberar uno la textura sigue viva para el otro. Liberar un texlist que no cargó su dueño (por ejemplo, el `texP` copiado en un clon) o uno viejo después de `njReleaseTextureAll` desajusta los contadores (textura liberada en uso o de otro). Las manos de los ficheros de arma usan 0x73 (Claire) y 0x1A5 (Chris); las texturas de arma (17.664 B) son comunes a los dos.
- **Índices globales de los personajes:** Claire 0x65-0x6A, 0x73 (también las manos de los ficheros de arma) y 0x1B8A (polilla); Claire B 0x96-0x9C; Chris 0x198-0x1AE; `[12]` 0x2BC-0x2C2; Wesker 0x384-0x3A2. No se solapan, así que dos personajes pueden tener las texturas cargadas a la vez.
- **VRAM:** `Ps2TexLoad` (ps2_dummy.c:1521-1575) sube cada textura a una dirección fija de VRAM cuando cambia, con una caché de una sola entrada. No hay colisiones en VRAM; más texturas distintas solo cuestan ancho de banda.

## Sonido

- `PlayerPos` se toma de `plp` (sdfunc.c:3010-3012).
- `CallPlayerFootStepSe` (player.c:1433) no recibe posición, así que suena donde está `plp`. Existe `CallPlayerFootStepSeEx(…, pPos)` (sdfunc.c:1447), que sí la recibe.

### Driver de sonido (IOP) y memoria SPU2

Investigado en octubre de 2026 para los sonidos de arma de P2. Lo del IOP sale de desensamblar `PS2_DATA/MODULES/TSNDDRV.IRX` ("TSND Ver 1.6", Tamsoft), que conserva la tabla de símbolos (`.symtab`) y `.mdebug`.

- **Cadena:** el EE (`ps2_sg_sd.c`, emulación de la API de sonido de Katana) encola órdenes con `Sdr*` (ps2_snddrv.c) y las manda por RPC a `TSNDDRV.IRX`, que usa `modhsyn`/`modmidi` de Sony.
- **Ocho puertos fijos** con zona propia en la RAM de la SPU2 (2 MB). Tablas `.data` del IRX: `Tsnd_spuadr_tbl` (9 entradas, la última es el final), `Tsnd_spusize_tbl` y `Tsnd_hd_size`. Todos los bucles del driver van hasta 8.

  | Puerto | Uso | SPU2 | Tamaño | Peor caso en disco | HD en el IOP |
  | --- | --- | --- | --- | --- | --- |
  | 0 | SE banco 0 (`COMMON.MLT`) | 0x020000 | 0xE400 | | 0x1000 |
  | 1 | MIDI banco 1 (`DOOR_xxx.SPQ`) | 0x02E400 | 0x15800 | 85.888 | 0x1000 |
  | 2 | MIDI banco 2 (sala) | 0x043C00 | 0x99800 | 626.624 | 0x4000 |
  | 3 | MIDI banco 3 (sala) | 0x0DD400 | 0x39000 | 231.296 | 0x1000 |
  | 4 | SE banco 1 (`ARMS_xxx.SPQ`) | 0x116400 | 0x1F000 | 125.888 (`ARMS_015`) | 0x1000 |
  | 5 | SE banco 2 (sala) | 0x135400 | 0x2D000 | 175.424 | 0x1000 |
  | 6 | SE banco 3 (sala) | 0x162400 | 0x77800 | 486.720 | 0x2000 |
  | 7 | SE banco 4 (`CORE_xxx.SPQ`, voz) | 0x1D9C00 | 0xD800 | 53.760 | 0x1000 |

  Cada zona tiene el tamaño de su peor caso: no sobra sitio dentro de ellas. El EE traduce banco → puerto con `SE_BANK = {0,4,5,6,7}` y `MIDI_BANK` (ps2_sg_sd.c, `sdMultiUnitDownload`). La tabla `IOP_hd_size` del EE (ps2_snddrv.c) solo sirve para calcular `iop_hd_adr`, que nadie lee.
- **Reverb:** `TsndLoop` pone el final del área de efectos en 0x1FFFF (núcleo 0) y 0x1FFFFF (núcleo 1). El juego solo usa los modos 0 y 5, Hall, que ocupa 0xADE0 bytes (`SdrSetRev`, ps2_sg_sd.c). El driver reserva sitio para el modo más grande (Echo, 0x18040), así que **quedan dos huecos libres**:
  - 0x050A0-0x1521F (≈ 64 KB); en 0x5080 hay un bloque del driver;
  - 0x1E7400-0x1F521F (≈ 55 KB). Empieza justo donde acaba el puerto 7 y coincide con `Tsnd_spuadr_tbl[8]`.

  En cinco savestates de juego los dos están a cero.
- **Formato de los `.SPQ`** (`MULTSPQ1/2.AFS`; `MULTSPQ?.IDX` es la lista de claves de `SearchAfsInsideFileId`): una tabla de `SPQ_HEADER` (`Offset`, `Size`, `Type`, `BankNo`) acabada en `Offset == 0`. `Type`: 0 secuencia MIDI, 1 programas MIDI, 2 banco de SE y 5 entorno de sala. Cada bloque de tipo 1 o 2 lleva `{hd_off, hd_size, bd_off, bd_size}`.
  - El HD es el formato JAM de Sony (`IECS` + chunks `Vers`, `Head`, `Vagi`, `Smpl`, `Sset` y `Prog`).
  - Las muestras de `Vagi` se dan como desplazamientos relativos al principio del BD.
  - El número de lista de un SE es el número de programa.
  - Un banco de armas tiene entre 1 y 7 muestras y unos 32 programas.
- **Carga de un banco de SE** (`sndr_trans_func`):
  1. Se manda el HD (`SdrHDDataSet2`, orden 0x29) y se comprueba su suma.
  2. Se manda el BD a trozos de 48 KB (`SdrBDDataTrans`, orden 0x2C). El IOP lo escribe en `Tsnd_spuadr_tbl[puerto & 0x7F] + desplazamiento`, que es de 24 bits, **sin comprobar el tamaño de la zona**.
  3. `SdrBDDataSet2` (orden 0x2B) llama a `sceHSyn_Load(puerto + 10, Tsnd_spuadr_tbl[puerto], HD, banco 0)`. Cada puerto tiene un único banco cargado, y `modhsyn` calcula la dirección de cada muestra como base + desplazamiento.
- **Reproducción:** la petición es `canal | puerto << 16 | programa << 8` (`sdShotPlay`). Hay 8 canales por banco de SE (`use_se_info`). `SetupSeGenericParm` saca el banco de los bits 8-11 del número de SE.
- **Lo que el IOP lee del HD de un banco de SE** (orden 0x29, en `treq_BGM`):
  - guarda una tabla por programa (`se_info`) para `Tsnd_tqreq`;
  - `se_max[banco]` es el último índice de `Prog`. Un programa mayor se ignora;
  - el driver supone que el primer split de cada programa indexa a la vez el sample set, el sample y la muestra (en todos los bancos del disco es así, salvo un desfase en `ARMS_015`);
  - la duración de cada muestra sale de la distancia a la siguiente entrada de `Vagi`, y la de la última, del tamaño del BD de `Head` (+0x10).
- **Banco de SE → puerto** en el IOP: `Tsnd_load_tbl` (16 bytes por puerto, byte 0xD = 0x80 | banco) llena `SE_TBL[banco]` con el puerto, su buffer de HD y `Tsnd_spuadr_tbl[puerto]`. El banco 4 es el puerto 7 (0x1D9C00).
- **Carga desde el EE:**
  - `ExecSoundSynchProgram` se ejecuta en la interrupción de VSync (`bhControlVSync`, sync.c), así que `LoadSoundPackFile` y las transferencias corren en paralelo al bucle del juego;
  - se pide un banco con `SpqKeyCode` (clave de `MULTSPQ?.IDX`: `0x4000|n` armas, `0x8000|n` puertas, `0xFFF0|n` voz, `etapa*1000 + sala*10 + caso` salas) y `SpqFileReadRequestFlag` (1 sala, 2 armas, 3 puerta, 4 voz). Quien pide espera antes a que valga 0 (`CheckTransEndSoundBank`);
  - mientras vale 2, `CallPlayerWeaponSeEx` no suena;
  - `TransSoundPackDataFlag` (`ExecTransSoundData`) no lo activa nadie.
- **Voz del jugador:** `CallPlayerVoice(SeNo)` (sdfunc.c) suena en el slot 7 con `PlayerPos` (la posición de `plp` copiada una vez por frame en `ExecSoundSystemMonitor`). El banco 4 (`CORE_xxx`, en `MULTSPQ1.AFS`, claves 0xFFF0-0xFFF3) tiene solo 4 programas con una muestra cada uno: 0x400 daño por detrás, 0x401 muerte, 0x402 daño leve y 0x403 daño fuerte. Los piden player.c (`bhCPM0_damage`, `bhCPM0_die`) y los enemigos que agarran o golpean al jugador (`bhEne_CallPlayerVoice`, que suma 0x400, y `bhEne_PlayerSePlay`). `CallPlayerVoice(519)` (en15.c) es del banco 2. BD de cada `CORE`: 53.760, 38.272, 44.672 y 50.432 B.
- **Cómo calcula `modhsyn` la dirección de una muestra** (`MODHSYN.IRX` del disco, sin símbolos; 0x3AD4 y 0x4F60): `sceSdSetAddr(voz | 0x2040, base + Vagi.desplazamiento)`, con una suma de 32 bits. Solo comprueba los índices y que el desplazamiento no sea 0xFFFFFFFF; no lo compara con el tamaño del BD. `sceSdSetAddr` (`LIBSD`, export 9) escribe `addr >> 17` en la parte alta sin máscara. Un desplazamiento "negativo" (0xFFE2B4A0 sobre 0x1D9C00) da exactamente 0x050A0.
- **Duraciones en `TSNDDRV`** (orden 0x29): `Vagi[i + 1] - Vagi[i]` por orden de índice, y la última con el tamaño del BD de `Head`. Un salto de direcciones entre dos entradas seguidas estropea la duración de la anterior. Las tablas de la pila admiten 128 muestras y 128 programas por puerto.
- **Subida de un BD** (`SdrBDDataTrans`): `Tsnd_spuadr_tbl[puerto & 0x7F] + desplazamiento` (24 bits), con el desplazamiento inicial `iop_trans_offset = 0` (ps2_sg_sd.c, `sdBankDownload`). `sceSdVoiceTrans` pone el TSA con el mismo `addr >> 17` sin máscara. Ninguna entrada de la tabla apunta por debajo de 0xD800, así que para escribir en 0x050A0 hay que dar la vuelta a los 2 MB: PCSX2 enmascara el TSA con 0xFFFFF (`DoDMAwrite`) y el SSA con 4 bits; el hardware real no está comprobado.
- **Voz de P2 (coop, coopsnd.c):** su `CORE` se pide con `SpqFileReadRequestFlag = 6`. G20 copia sus entradas y reescribe sus Vagi como 0xFFE2B4A0 + desplazamiento; G28 (ps2_sg_sd.c) hace que su BD se suba al puerto 7 con desplazamiento inicial 0x2B4A0 (0x2050A0, que la SPU2 lleva a 0x050A0, el hueco bajo). El HD del banco 4 queda así: voz de P1 (listas 0-3), voz de P2 (listas 4-7), armas de P2 (32 + lista); por índices, voz de P1, armas de P2, una entrada falsa que marca el final de lo anterior y voz de P2, y `Head` lleva 0xFFE2B4A0 + el tamaño de la voz de P2. G23 (`CallPlayerVoice`) lleva a 4 + lista las voces pedidas con `plp == &ply2` si el personaje de P2 no es el del último `CORE` de P1.
- **Medir:** en un savestate de PCSX2, `SPU2.bin` lleva la RAM de sonido a partir del byte 0x10004 y `iopMemory.bin` la RAM del IOP. La base de `TSNDDRV` se encuentra buscando "TSND Ver 1.6" (dirección del módulo 0x18B59). En la parte de `SPU2.bin` anterior a 0x10004 están las direcciones de inicio de las voces en medias palabras.

## Interfaz y salud

- No se muestra la vida durante el juego; solo en la pantalla de estado: `StatusInit` (sub1.c:1389) lee `plp->hp`/`stflg`, y `Pulse*` (sub1.c:9075-9275) dibuja el ECG.
- La curación está en `Use_00` (sub1.c:6283).
- La muerte normal lanza el game over poniendo a 0 el bit `0x4000` de `ts_flg` (`bhCPM0_die`, player.c:6424). `gm_flg 0x400` indica otro caso: game over por fin de una cuenta atrás (player.c:1298).
