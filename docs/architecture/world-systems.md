# Sistemas del mundo: cámara, colisión, objetos, luces y sonido

Rutas relativas a `src/ps2/veronica/prog/`. Cada sección indica si el sistema recibe la entidad como **parámetro** o usa el **global `plp`**. Esto es lo que decide si un sistema sirve tal cual para un segundo personaje.

## Cámara

Hay una sola cámara global, `CAM_WORK cam` (types.h:1062, main.c:39). No hay pantalla partida.

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

### Texturas

- **Pool:** `Ps2_tex_mem[10485760]` (ps2_dummy.c:35, 79), lineal, con 256 entradas `tbuf` (main.c:34, 160; `njInitTexture`, ps2_NaTextureFunction.c:60-144). Las texturas de render ocupan 1 MB cada una (ps2_NaTextureFunction.c:558-597).
- **Carga:** `bhSetMemPvpTexture(texP, datp, 0)` (ps2_texture.c:148-188) recorre los TIM2 de un bloque y llama a `njLoadTexture` (ps2_NaTextureFunction.c:204-336) para cada uno. **Se deduplica por índice global** (el `Gindex` del TIM2): si ya hay una textura con ese índice, se incrementa su contador y se reutiliza. Si no, `Ps2TextureMalloc` (:552-634) **copia** el TIM2 al pool, así que el buffer de lectura se puede tirar después.
- **Si el pool o las entradas `tbuf` se llenan, el juego hace `exit(0)`** (ps2_NaTextureFunction.c:286-289, 321-324). No hay `printf` del espacio libre: está en `Ps2_free_texmemsize` (ps2_NaTextureFunction.c:29).
- **Liberar:** `njReleaseTexture(texP)` decrementa el contador y libera al llegar a 0 (:401-455); después `bhGarbageTexture(NULL, 0)` compacta el pool (`Ps2TextureGarbageCollectionAll`, :719-769), como hace el cambio de personaje (system.c:1739-1747). `njReleaseTextureAll` lo reinicia todo.
- **Índices globales de los personajes:** Claire 0x65-0x6A, 0x73 (también las manos de los ficheros de arma) y 0x1B8A (polilla); Claire B 0x96-0x9C; Chris 0x198-0x1AE; `[12]` 0x2BC-0x2C2; Wesker 0x384-0x3A2. No se solapan, así que dos personajes pueden tener las texturas cargadas a la vez.
- **VRAM:** `Ps2TexLoad` (ps2_dummy.c:1521-1575) sube cada textura a una dirección fija de VRAM cuando cambia, con una caché de una sola entrada. No hay colisiones en VRAM; más texturas distintas solo cuestan ancho de banda.

## Sonido

- `PlayerPos` se toma de `plp` (sdfunc.c:3010-3012).
- `CallPlayerFootStepSe` (player.c:1433) no recibe posición, así que suena donde está `plp`. Existe `CallPlayerFootStepSeEx(…, pPos)` (sdfunc.c:1447), que sí la recibe.

## Interfaz y salud

- No se muestra la vida durante el juego; solo en la pantalla de estado: `StatusInit` (sub1.c:1389) lee `plp->hp`/`stflg`, y `Pulse*` (sub1.c:9075-9275) dibuja el ECG.
- La curación está en `Use_00` (sub1.c:6283).
- La muerte normal lanza el game over poniendo a 0 el bit `0x4000` de `ts_flg` (`bhCPM0_die`, player.c:6424). `gm_flg 0x400` indica otro caso: game over por fin de una cuenta atrás (player.c:1298).
