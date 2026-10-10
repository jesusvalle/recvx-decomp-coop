# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Qué es este repo

Decompilación "matching" de **Resident Evil: Code Veronica X** para PS2 (versión US, `SLUS_201.84`). El juego entero está reconstruido en C: no queda ASM sin decompilar en `src/ps2/veronica/prog/`. El ejecutable se recompila con MWCC (Metrowerks CodeWarrior para PS2), se mete en la ISO original y se prueba en PCSX2.

Este clon es el **fork del usuario para hacer un modo cooperativo** (segundo jugador con el mando 2). Decisiones, alcance del hito actual y riesgos: [docs/coop/README.md](docs/coop/README.md). El usuario escribe en español.

Lo específico de la máquina del usuario (rutas del emulador y del SDK) está en `CLAUDE.local.md`, que no se sube.

### Remotos

- `origin` = `jesusvalle/recvx-decomp-coop`, el fork, que es **público**: no subas ISOs, ejecutables del juego ni el SDK.
- `upstream` = `AshfordFamily/recvx-decomp`, el original.

`master` es una copia exacta del original: no se hacen commits en ella. El trabajo va en ramas (`coop-hito1`…). La rama por defecto del fork en GitHub es `coop-hito1`, para que se vea el aviso del `README.md` (fork experimental hecho con IA); si se cambia de rama de trabajo, cambia también la rama por defecto. Para traer cambios del original: `git fetch upstream`, `git merge upstream/master` en `master`, `git push origin master` y, después, `git merge master` en la rama (merge, no rebase: las ramas ya están subidas). Compila después de cada actualización: si el original renombra algo que usa `coop.c`, el merge sale limpio pero la compilación falla.

## Documentación de arquitectura

El conocimiento del motor está en [docs/architecture/](docs/architecture/README.md). Empieza por su README, que tiene una tabla de "¿dónde está…?", los globales, las convenciones de nombres y recetas de búsqueda.

**Mantenlo al día:** si descubres algo del motor que no esté documentado, o una referencia `archivo:línea` que ha quedado mal, corrige el documento correspondiente en el mismo cambio. Cuando avance el cooperativo, actualiza la tabla de estado y las decisiones de `docs/coop/README.md`.

## Puesta en marcha

Pasos para montar el entorno desde cero:

1. `git submodule update --init --recursive`. Trae `include/recvx-decomp-mwcc` (el compilador), `-katana` y `-cri`.
2. El SDK de PS2, que no viene en el repo. El 2.0.0 va en `include/recvx-decomp-ps2_sdk/` y el 3.0.3 en `include/recvx-decomp-ps2_sdk303/`. Las rutas exactas están en `compile_config.json`.
3. Copiar `SLUS_201.84` del disco a `config/`.
4. `pip install -r config/requirements.txt` (instala splat). Hay alternativa con dev container en `.devcontainer/`.
5. `python compile.py --setup`: genera `objdiff.json`, `build/expected/` y `config/asm/`.

En Linux hace falta `wibo` para ejecutar `mwccps2.exe`.

### Estado del entorno (Windows, verificado el 2026-10-09)

Todo montado y probado. **Usa el build por defecto (SDK 2.0 + librerías originales embebidas), no `--sdk303`.** El ejecutable compilado sin cambios juega en PCSX2 con los vídeos funcionando.

- **Python:** usa `.venv/Scripts/python.exe`; el Python del sistema no tiene splat.
- **SDK, PCSX2 y rutas locales:** en `CLAUDE.local.md`.
- **Arreglo para Windows:** `compile.py` lleva 3 líneas en `run_command` (no están en el original) para que Windows encuentre los ejecutables con rutas relativas.
- **ISO:** la original está en `iso/` y extraída en `iso/data/`.
- **El ELF no es idéntico byte a byte al retail** (CRC de PCSX2 `D20D9EC2` frente a `24036809`). Es normal: el proyecto reproduce cada función, no el archivo entero.

**`--sdk303` deja los vídeos en negro.** Dos causas:

1. El `ioprp300.img` delega el streaming de CD en el dispositivo `cdrom_stm0:` de `cdvdstm.irx`, y el juego nunca carga ese módulo. El IOP muestra `Unknown device 'cdrom_stm'` y `sceCdStRead` no devuelve datos.
2. `libmpeg` 3.0.3 exige `w*h*9/2 + 10240` bytes de trabajo. `initAll` (ps2_MovieFunc.c:69) pasa 508928 fijos, que es el requisito de `libmpeg` 2.0 para los vídeos de 320x352 (`MV_000`, `014`, `016`, `021`).

## Comandos

En esta máquina, `python` = `.venv/Scripts/python.exe`.

```sh
python compile.py                    # compila y enlaza → elf/main.elf (errores en elf/report.txt; los avisos de const son normales)
python compile.py --verbose          # muestra cada comando
python compile.py --single-file build/src/ps2/veronica/prog/player.o   # compila un objeto sin enlazar (lo usa objdiff)
python compile.py --progress         # compila también los objetos esperados y genera report.json con objdiff-cli
python compile.py --sdk303           # con el SDK 3.0.3: los vídeos no funcionan, no usar

python mkiso.py -m extract --iso "iso/Resident Evil Code Veronica X.iso"   # extrae la ISO (ya hecho)
python mkiso.py -m insert            # mete elf/main.elf como SLUS_201.84 → iso/RECVX_NEW.iso (sin --sdk303)
```

Si cambias de SDK (de 2.0 a 3.0.3 o al revés), borra antes `build/src/`. La compilación incremental no detecta el cambio de cabeceras.

Para arrancar el juego en PCSX2: `pcsx2-qt.exe -fastboot -- iso/RECVX_NEW.iso` (la ruta del emulador y el comando completo están en `CLAUDE.local.md`).

El teclado no está asignado a ningún mando: para pasar del título hace falta que el usuario juegue con el suyo.

**No hay tests automáticos.** Para comprobar un cambio: compilar sin errores, generar `RECVX_NEW.iso` y probarlo en PCSX2. Los cambios que afectan al mando 2 necesitan dos mandos configurados en PCSX2. Para saber si una función sigue siendo idéntica al original se usa objdiff con `objdiff.json`.

## Reglas para cambiar código

- **Código cooperativo detrás de `#ifdef COOP`**, preferiblemente en archivos nuevos. Compilando sin `COOP`, el código del juego debe salir idéntico al del build sin el mod (se comprueba con objdiff). Con `--sdk303` el ejecutable nunca es idéntico byte a byte al retail, porque cambian las librerías. Los defines globales se añaden a la lista `"defines"` de `compile_config.json` (ahora tiene `DEBUG`, `COOP` y `COOP_SPLIT`).
- **Un `.c` nuevo hay que añadirlo a `"source_files"`** de `compile_config.json`. Si está en `src/ps2/veronica/prog/` usa automáticamente MWCC con `sdata = 0` (`source_overrides`). El enlazado es reubicable (`config/SLUS_201.84.lcf`, `ORIGIN 0x100000`, heap `AFTER(main)`), así que añadir código no rompe direcciones.
- **No cambies la estructura de `SYS_WORK` ni de `BH_PWORK`.** El rango `sys->version..save_end` es el formato de la partida guardada, del reintento y de las demos (se copia en bruto). Además, `bhInitSystem` borra un tamaño escrito a mano. El estado nuevo va en globales nuevas.
- **Imita el estilo del archivo que tocas.** Las variables se declaran al principio de la función y los flags se escriben en hex. Las llaves varían según el archivo (Allman en la mayoría; K&R en algunas funciones como `bhAllDrawModel`). Los comentarios `// 100% matching!` se reservan para funciones originales.
- Si algún día se manda código al repo original (AshfordFamily/recvx-decomp), su README pide declarar el código generado por IA.

## Cooperativo (código)

- **Dónde está:**
  - `src/ps2/veronica/prog/coop.c` + `include/ps2/veronica/prog/coop.h`;
  - el bloque `#ifdef COOP` al final de `ps2_sg_pad.c` (lectura del puerto 2);
  - el bloque `#ifdef COOP` al final de `ps2_sg_pdvib.c` (vibración del mando 2);
  - `src/ps2/veronica/prog/coopsnd.c`: sonidos del arma de P2 (su banco reducido en el banco de SE 4 y un hueco libre de la SPU2);
  - `src/ps2/veronica/prog/coopcam.c` + `include/ps2/veronica/prog/coopcam.h`: pantalla partida (hito 5, define `COOP_SPLIT`);
  - ganchos pequeños (G1-G3, G5-G9 y G11-G35; G10 no existe y G4 se quitó en el hito 7), listados en [docs/coop/README.md](docs/coop/README.md).
- **Cómo funciona:** P2 es `BH_PWORK ply2`. Se actualiza con el `bhControlPlayer()` original dentro de `coopBegin()`/`coopEnd()`, que intercambian el mando, ponen `plp = &ply2`, y guardan y restauran `st_flg`, `cb_flg`, `gm_flg`, `pt_flg`, `flr_idx`, `etc_idx`, `pl_htp`, `door` y `cam`.
- **Regla:** cualquier llamada nueva que use `plp` por dentro (por ejemplo `bhCheckWallEx`) sobre P2 va dentro de ese contexto.
- **Activar o desactivar:** `"COOP"` en `defines` de `compile_config.json`. Al cambiarlo, borra `build/src/`.
- **P2 desde el hito 2a:** lleva su propio modelo (Claire B, cargado en el paso 10 del modo 1 por G8) y su coleta, en objetos propios de `coop.c`.
- **Combate de P2 (hito 2b):** `coopBegin`/`coopEnd` protegen también el objeto de arma, los impactos del frame, el fogonazo y el puerto de vibración (ver [combat.md](docs/architecture/combat.md)).
- **Arma propia de P2 (hito 2c):** su inventario es `sys->itm[256..271]` (metadatos en `272..279`, dentro de la partida guardada). Su arma la carga `coopReadWeapon2Data` (G8 y, para el inventario de P2, el modo 3 con `mn_md3 = COOP_MN_P2`, G11) en buffers propios; desde el hito 7, también sus animaciones de cuerpo y sus datos z. Dentro de `coopBegin`/`coopEnd`, `swork.pip` y los bits `gm_flg 0x40000`/`0x10000000` son los de P2.
- **Inventario de P2 (hito 2d):** la pantalla original con un dueño (`coop_inv_owner`): G13 abre la de P2 con su Start o su petición, G14 pone su bloque, G15 pone `plp = &ply2` solo alrededor de `ItemTaskCheck`/`StatusMain` y es el único punto de cierre, G16 bloquea los objetos clave. La acción de P2 es un sondeo propio de zonas de objeto y baúl (`coopActionP2`), no `bhCheckExmAtari`.
- **Salud y enemigos (hito 3):** la vida de P2 está en `itm[273]` (veneno en `274`). G17 (eneset.c) cambia `plp`, el mando y el puerto de vibración a P2 durante el update de los enemigos que le eligen (`coop_tgt[]`); G18 hace lo mismo para los efectos dañinos; G19 repite sobre P2 el daño de las explosiones. A P2 muerto no se le reanima.
- **Personaje de P2 y mercenarios (hito 7):** P2 puede ser cualquier personaje (`coop_p2_id`/`coop_p2_cos`; en la historia, siempre Claire con el otro traje). Dentro de sus contextos (`coopBegin`/`coopEnd`, G15, G17), `sys->ply_id`, `sys->costume` y `sys->plzmtp` son los de P2 (`coopSwapId`). En mercenarios, la selección (`RM_5500`, un guion) se repite para P2 redirigiendo su puerta (G24) y desviando traje, personaje e inventario (G25-G27); P2 se carga en la primera sala (G22).
- **Pantalla partida (hito 5, prototipo):** con `"COOP_SPLIT"` en `defines` (requiere `COOP`), en juego normal la pantalla se parte en dos franjas de 640x240, cada una con el plano de su jugador. La cámara de P2 (`coop_cam2`) se calcula con el `bhCheckCut` original en un cambio de contexto (G29), y G30 dibuja dos pasadas de `bhAllDrawModel` con recorte del GS. L1+L2+R1+R2 del mando 1 alterna una o dos cámaras (`itm[275]`). G32 lleva los bits de primera persona de P2 en `coopBegin`/`coopEnd`. Ver [Hito 5](docs/coop/README.md#hito-5-pantalla-partida-prototipo).
- **Escaleras y eventos (hito 8):** el botón de acción de P2 pasa por el `bhCheckExmAtari` original, filtrado por G33 a escaleras y salientes; P2 tiene su propio `sys->pl_htp` (`coop_pl_htp2`, en `coopBegin`/`coopEnd`). Su activador de suelo se pasa al guion si P1 no pisa ninguno (`coopTriggerP2`), y G34 intercambia a P1 y P2 si el evento toma el control.
- **`COOP_TEST`:** define solo para pruebas (nunca en el build normal): pone una pistola con 15 balas, equipada, en el bloque de P2 al cargar. Se activa con `#define COOP_TEST` tras el `#ifdef COOP` de coop.c. Con `#define COOP_TEST_P2_ID n` además, en la historia P2 es el personaje `n` (1 Chris, 2 Steve, 3 Wesker). `#define COOP_TEST_SCOPE` (hito 5) da el lanzador lineal equipado a P2 y otro, sin equipar, a P1.
- **Specs y planes:** `docs/superpowers/specs/` y `docs/superpowers/plans/`.
- **Cómo probar** (herramientas, navegación en PCSX2, partida de prueba, identidad sin `COOP`): [docs/coop/testing.md](docs/coop/testing.md).

## Modelo mental del motor

- **Globales** (main.c): `sys` (`SYS_WORK*`: flags, mando, memoria, partida), `rom` (`ROM_WORK*`: tablas de la sala), `ply` + **`plp`** (el jugador), `ene[128]` (enemigos y NPCs), `eff[512]`, `cam`.
- **Bucle:** `njUserMain` ejecuta las tareas de `bhSysTaskJumpTab[23]` cuyo bit está activo en `sys->tk_flg` y no suspendido en `ts_flg`. Un frame de juego es `bhMainSequence` (game.c:20): enemigos → jugador → efectos → objetos → cámara → luz → dibujo. Los scripts de evento van en la tarea 8.
- **`BH_PWORK`** es la entidad universal (jugador, enemigos, NPCs), con una máquina de estados `mode0..mode3` y bloques extra `exp0..exp3`.
- **El jugador es un global.** Casi todo player.c son funciones `void f(void)` que usan `plp` y leen el mando de `sys->pad_*`. `sys->plp` existe pero nadie lo lee.
- **Memoria:** asignador lineal (`bhGetFreeMemory` avanza `sys->memp`). Al cambiar de sala vuelve a `sys->mempb`: lo que está debajo de `mempb` (los buffers del jugador) sobrevive; `ene[]`, `eff[]` y todo lo demás se pierde.
- **Flags:** son literales hex sin nombre (`sys->sp_flg & 0x1` = el jugador se actualiza). Glosario en [docs/architecture/events-and-flags.md](docs/architecture/events-and-flags.md).

## Trampas conocidas

- `pdGetPeripheral(port)` devuelve un único periférico estático y solo refresca una vez por frame. El puerto 1 nunca se lee (`Ps2_pad_read` está fijado al 0).
- `bhMlbBinRealize` reubica punteros sobre los propios datos: no hay que llamarlo dos veces sobre el mismo binario.
- La pose de animación se escribe en el árbol de huesos del modelo (`objP`). Dos instancias no pueden compartir ese árbol.
- Varias funciones reciben un `BH_PWORK*` pero leen `plp` igualmente: `bhCheckFloorP`, el daño de `bhCheckWall*` y `bhCheckExmAtari`.
- Los ids de entidad 31-38 de `bhJumpEnemy[]` los sustituyen algunos enemigos al inicializarse.
- Los números de línea de `docs/` pueden desplazarse con los cambios: busca por el nombre de la función.
- **MWCC no es determinista con `ps2_SystemSaveScreen.c`, `player.c`, `effsub1b.c`, `adv.c`, `light.c` ni `screen.c`**:
  - en `ps2_SystemSaveScreen.c` (`DispSysSaveMessageSelect`) cambia el orden de dos constantes float;
  - en `player.c` (`bhCPM2_act_wlk`) cambia el registro float elegido (`$f12`/`$f13`). Seis compilaciones del mismo fuente dieron cuatro objetos distintos;
  - en `effsub1b.c` (`bhDraw137`) cambia hasta el tamaño de la función (8 bytes), lo que desplaza todo lo que va detrás. Cinco compilaciones dieron cuatro objetos distintos.
  - en `adv.c` cambian unos bytes de `DisplayOptionBg` (visto en el hito 7: tres recompilaciones hasta coincidir).
  - en `light.c` (`bhSetHalfLight`) y `screen.c` también cambian bytes (visto en el hito 5). En la prueba de identidad se recompila cualquier objeto que difiera, hasta que coincida (23 intentos en el peor caso).

  Para comparar dos ELF byte a byte, borra el `.o` y recompila hasta que coincida (`rm build/src/ps2/veronica/prog/player.o` y `compile.py`). Para saber qué objeto difiere, compara las tablas de símbolos de los dos ELF: el primer símbolo con otra dirección o tamaño lo indica. Compara solo los segmentos `PT_LOAD`; la información de depuración cambia con cualquier línea nueva.
- **Con el build por defecto (SDK 2.0), los `printf` del juego no salen en el log de PCSX2**, ni con la consola del EE ni con la del IOP, y `njPrintC` está vacío. Para verificar en tiempo de ejecución:
  - capturas y el log propio de PCSX2 (por ejemplo, `Pad: DS2 Config Finished - P2/S1` demuestra que se sondea el puerto 2);
  - **leer la RAM desde un savestate**: con `SavestateCompressionType = 0` en `PCSX2.ini` el `.p2s` es un zip sin comprimir con `eeMemory.bin` (32 MB, dirección del EE `& 0x1FFFFFF`). Las direcciones de los globales (incluidos los `static`) están en `elf/main.elf.xMAP`. La herramienta del hito 2a es `.superpowers/sdd/2026-10-10-coop-hito2a/tools/ramread.py`, y el arnés `pcsx2.ps1` tiene `SaveState` (tecla F1).
- **El puerto 2 solo se lee durante el juego**, porque la tarea 6 (`bhSysCallPad`) no está activa en logos ni en el título. Nadie más llama a `pdGetPeripheral(1)`.
- **`mkiso.py -m insert` falla en silencio si PCSX2 tiene abierta `iso/RECVX_NEW.iso`.** Cierra PCSX2 antes y comprueba que la fecha de la ISO es posterior a la de `elf/main.elf`.
- **`plp->flg & 0x10000` no significa "controlado por guion"**: lo ponen también la animación de espera, el empuje y el daño. Para detectar guiones usa `mode0 == 7`.
- **`sys->pad_oncpy` es el historial de pulsaciones:** `bhSetPad` calcula `pad_ps = pad & ~pad_oncpy` (pad.c:275) y nadie más lo lee. Si se enmascara (como hace `coopSetPad2` con el mando de P2), los botones enmascarados dan una pulsación nueva en cada frame que se mantienen. Por eso el mando de P2 enmascara `on/ps/rs/old` pero no `oncpy`.
- **Muertes en agarres:** los enemigos ponen `hp < 0` dentro del agarre (con `stflg 0x40000`) y terminan la secuencia leyendo `plp`; si `plp` cambia de jugador a mitad, enemigo y jugador se quedan congelados sin game over. Ver [enemies-and-npcs.md](docs/architecture/enemies-and-npcs.md).
- **`grep -c $'$'` en Git Bash da recuentos falsos de CRLF**: para saber los finales de línea de un archivo, cuenta `b'
'` con Python. Los archivos del repo no son todos iguales (unos CRLF, otros LF): al editar con scripts, detecta el final de línea de cada uno.
- **El heap de la libc empieza en una dirección fija (0x1E2CD00, el `end` del retail).** splat dejó la base de `sbrk` (`heap_ptr.30` en glue.s) como constante. Si el BSS crece por encima, `malloc` y el búfer de `printf` caen dentro de las variables del programa. Con `COOP` lo corrige `coopFixHeap` (G35, en `main`). Si algún día cuelga la carga tras cambiar solo el tamaño del binario, mira esto primero. Ver [rooms-and-memory.md](docs/architecture/rooms-and-memory.md#memoria).
- **Pruebas automáticas en PCSX2:** se pueden añadir asignaciones de teclado a `[Pad1]`/`[Pad2]` de `PCSX2.ini` (líneas extra con la misma clave, sin quitar los mandos), enviar teclas con `keybd_event` a la ventana y capturar con `PrintWindow`. Haz copia del ini y restáuralo al acabar. **No pulses Alt** para dar el foco: Alt+Enter cambia a pantalla completa.
