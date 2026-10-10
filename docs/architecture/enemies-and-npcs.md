# Enemigos y NPCs

Rutas relativas a `src/ps2/veronica/prog/`.

## El sistema `ene[]` ([eneset.c](../../src/ps2/veronica/prog/eneset.c))

- `BH_PWORK ene[128]` (main.c:41) guarda **todos** los enemigos, NPCs, partes de enemigos y algunos objetos con lógica de la sala.
- **Update:** `bhControlEnemy` (eneset.c:344) recorre `ene[]` y llama a `bhJumpEnemy[ep->id](ep)` (tabla en eneset.c:51). Solo se ejecuta si `sys->sp_flg & 0x2`, y va **antes** que el jugador en cada frame.
- **Dibujo:** `bhDrawEnemy` (eneset.c:797) llama a `bhPutModel(ep)` para cada entrada. Las entradas con `flg & 0x80000000` se dibujan en cambio desde `sys->en_obj[16]` / `bhDrawEneObject` (eneset.c:866).
- **Tabla de ids:** 0-30 → `bhEne00..30`; 53-56 → `bhEne53..56`; 71 → `bhEne71`; el resto de 31 a 99 → `bhSubpl` (NPCs de guion).
- **Ojo con los ids 31-38:** algunos enemigos, al inicializarse, sustituyen esas entradas de la tabla por el handler de sus partes con `bhEne_SetCallFunc`, y nadie restaura `bhSubpl`. Lo hacen en01.c:2605-2669 (31-38), en02.c:804, en03.c:733, en05.c:1023, en06.c:360, en13.c:193, en17.c:827/829 y en23.c:514. Hay que tratar esos ids como ocupados.
- **Todo `ene[]` se borra en cada carga de sala** (`bhInitEnemy`, desde `bhInitReadRDT`).

### Crear entidades

| Función | Línea | Qué hace |
| --- | --- | --- |
| `BH_PWORK* bhSetEnemy(EGG_WORK* etp, int)` | eneset.c:170 | Ocupa el primer hueco libre de los 128, lo pone a cero y copia `flg` (el bit 0 tiene que estar a 1), `id`, `type`, `flr_no`, `mdlver`, `px/py/pz` y `ay`. Devuelve NULL si `ene[]` está lleno. `EGG_WORK` es `ETTY_WORK` (types.h:1287). Ids > 40: activa `mdflg` 0x20 y `flg` 0x8000. Ids > 90: activa la máscara facial. **No configura ni modelo ni animaciones:** hay que poner `mlwP` y `mnwP` a mano, o `bhDrawEnemy` desreferenciará un `mlwP` nulo. |
| `void bhEne_SetCallFunc(void(*)(BH_PWORK*), unsigned id)` | eneset.c:933 | Sustituye el handler de un id en `bhJumpEnemy[]`. |
| `void* bhEne_CallocWork(int size, int)` | eneset.c:906 | Memoria de trabajo para la entidad. Dura lo que dura la sala. |

Ejemplos de creación en tiempo de ejecución:

- `bhCheckMothEgg` (player.c:1042) crea el id 27.
- **`en27.c:61-66`** toma prestado el modelo y las animaciones del jugador: `mnwP = sys->plmthp`, `mlwP = &plp->mdl[6]`.

Las entidades de la sala salen de `rom->enep` en `bhSetRoom` → `bhSetEneMdl` (room.c:559). Sus animaciones se cargan en `sys->emtp[id]` con `bhSetEneMtn` (room.c:647).

### Dependencia del jugador

- Toda la IA usa el global `plp` directamente, sin puntero de "blanco": unas 3.000 referencias en 31 archivos `enNN.c` (columna `plp` de la tabla).
- **Ayudas compartidas que usan `plp`:** zonzon.c, zonzon1.c, `bhSearchPlayer` (pwksub.c:664) y `bhCheckPlayer` (hitchk.c:5712).
- **Ayudas que reciben parámetros:** `bhEne_AttackHitCheck` (zonzon1.c:611) y `bhCheckRoute` (rutchk.c:5).
- **Agarres:** los `_PlayerControl` / `_Nage` mueven a `plp` y escriben `sys->pad_on`.
- **Ningún enemigo guarda a quién ataca:** todos leen `plp` en cada frame. El agarre del zombi vive en flags privados del enemigo (`EXP0_I(0x40)`: `0x80` agarrando, `0x20000` controla al jugador, `0x80000` lo coloca) más el `mode0` del jugador; si `plp` cambiara a mitad, el enemigo movería al otro. Excepciones con puntero guardado (`lkwkp = plp`): el gusano (en01sub.c:1417), la larva de polilla (en27.c:84) y los efectos effsub2.c:3188 y 4266.
- **Nadie usa el global `ply` ni compara con `&ply`** en enemigos, zonzon, eneset, efectos, weapon, hitchk ni pwksub (comprobado con grep). rutchk.c no lee `plp`.
- **Más funciones compartidas que leen `plp`:** `bhCheckEnemies` (hitchk.c:5788), `bhEne_CheckEnemiesBall` (zonzon1.c:529), `bhSearchNearEnemy2` (pwksub.c:395), sangre y trozos de carne (zonzon.c:462, zonzon1.c:73, 114, estéticos) y en15 `target_direction/distance` (en15.c:265, 281).
- **Los enemigos no comprueban `stflg 0x1000000`** (jugador oculto): solo `0x80000000` (en puerta), `flg 4/2` y `hp`.
- **Estado global que escriben:**
  - el mando: `sys->pad_on &= ~0xF` al soltar un agarre, y `sys->pad_ps` en el machaque de botones (ver [combat.md](combat.md#daño-al-jugador));
  - temblores de cámara (`cam.ofx/ofy/ofz`, en02, en13, en17) y `sys->rm_flg`;
  - en15 y en25 quitan la mira y la primera persona (`gm_flg`/`st_flg`/`pt_flg`); en25:207 hace `pt_flg &= ~0x1`, que oculta el dibujo de los jugadores;
  - en15:3325 y 3345 ponen `ts_flg |= 0x4000`.
- **La larva y el huevo de polilla** (en27) usan los modelos 6 y 7 del jugador (en27.c:61-66, 202), que solo trae el fichero de la Claire normal.
- **Segundo jugador (build `COOP`, hito 3):** G17 (eneset.c) envuelve `bhJumpEnemy[ep->id](ep)` con `coopEnemyBegin`/`coopEnemyEnd`. Como la IA solo lee `plp`, basta con poner `plp = &ply2` (más el mando de P2 y `CurrentPortId = 1`) durante el update de los enemigos cuyo objetivo es P2. La tabla `coop_tgt[128]` guarda el objetivo; solo eligen los ids 1, 3, 4, 5, 7, 9, 10, 21, 22, 23, 24, 26 y 30, y no cambian con `mode0 != 1` ni con el objetivo en `mode0` 2/4/5/6. Las partes (`flg & 0x80`) siguen a su `lkwkp`. **El bloqueo prevalece sobre la validez de P2:** los agarres mortales ponen `hp < 0` dentro del agarre (con `stflg 0x40000`) y su `_PlayerControl` tiene que seguir viendo al mismo `plp` hasta poner `flg |= 2`; si el enemigo cambiara a P1 en ese momento, enemigo y P2 se quedarían congelados sin game over. Detalle en [../coop/README.md](../coop/README.md).
- **Disparos del jugador:** el apuntado automático y los impactos buscan entradas de `ene[]` con `flg & 0x20` (playpch.c:349, weapon.c:615-619). Detalle en [combat.md](combat.md).
- **Del impacto a la IA:** el disparo (`bhCheckGunAtari`) solo anota el impacto en el enemigo (`flg |= 0x4`, `dpx/dpy/dpz`, `dvx/dvy/dvz`, `dam[]`…). El enemigo lo procesa en su siguiente update y borra `flg 0x4`; después su IA usa `plp` (ejemplos: en01.c:8228, en05.c:3032). Solo cuenta un impacto por enemigo y frame. Ver [combat.md](combat.md).

## Catálogo

Los nombres salen de los comentarios `// ENEMY: …` que los autores del decomp pusieron al principio de cada `enNN.c`, contrastados con el código. La columna `plp` cuenta los tokens `plp` del archivo.

| id | Función | Archivo:línea | Entidad | Confianza | Entradas clave | `plp` |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | `bhEne00` | eneset.c:391 | Objeto explosivo (bidón o similar) | probable | switch en `mode0` | 0 |
| 1 | `bhEne01` | en01.c:2467 | Zombi (31 variantes de modelo) | segura | `Init` :2532, `MainLoop` :2504, `Brain00` :4603 | 156 |
| 2 | `bhEne02` | en02.c:714 | Gulp Worm (gusano de arena) | segura | `BR00` :857, `PlayerControl` :2661 | 350 |
| 3 | `bhEne03` | en03.c:478 | Black Widow (araña) | segura | `Init` :625, `PlayerControl` :6798 | 294 |
| 4 | `bhEne04` | en04.c:516 | Cerberus (perro zombi) | segura | `Init` :939, `Brain` :1420 | 172 |
| 5 | `bhEne05` | en05.c:901 | Hunter / Sweeper | segura | `BR00` :1079 | 193 |
| 6 | `bhEne06` | en06.c:229 | Polilla gigante (su agarre pone un huevo: id 27) | segura | `NG00` :1079 | 70 |
| 7 | `bhEne07` | en07.c:109 | Murciélago | segura | `PlayerControl` :2172 | 110 |
| 8 | `bhEne08` | en08.c:36 | Pez (decorativo, inofensivo) | probable | `MV00..02` | 0 |
| 9 | `bhEne09` | en09.c:2037 | Bandersnatch | segura | `Brain` :3048 | 176 |
| 10 | `bhEne10` | en10.c:30 | Insecto pequeño ("Ant A") | probable | `Brain` :159 | 16 |
| 11 | `bhEne11` | en11.c:52 | "Spotter": vigía con foco que trepa por paredes; activa `rm_flg` bit 0 al ver al jugador | probable | `BR00` :216, `CameraSet` :1027 | 11 |
| 12 | `bhEne12` | en12.c:478 | Alexia, primera forma | segura | `BR00` :592 | 181 |
| 13 | `bhEne13` | en13.c:50 | Alexia, segunda forma (cuerpo enraizado) | segura | `Init` :63, `SelectTentacle` :1137 | 100 |
| 14 | `bhEne14` | en14.c:180 | Alexia, tercera forma (alada) | segura | `Init` :225 | 78 |
| 15 | `bhEne15` | en15.c:362 | Nosferatu | segura | `Init` :423, `Attack` :940, `CheckDamage` :1564 | 326 |
| 16 | `bhEne16` | en16.c:38 | Alfred Ashford, francotirador (invisible; mira láser) | segura | `BR00` :106, `MV02` :154 | 26 |
| 17 | `bhEne17` | en17.c:347 | Steve monstruo (hacha) | segura | `Init` :789, `Brain` :927 | 42 |
| 18 | `bhEne18` | en18.c:359 | Tentáculos y cajas de impacto de Alexia 2 | segura | `Init` :373 | 0 |
| 19 | `bhEne19` | en19.c:226 | Tyrant T-078 (tipo 0 probablemente Rockfort; 1-2 el avión) | segura | `Br00` :543, `Br01` :982, `Br02` :1539 | 58 |
| 20 | `bhEne20` | en20.c:33 | Criatura decorativa ("Ant B") | sin confirmar | `MV00..07` | 0 |
| 21 | `bhEne21` | en21.c:88 | Albinoid cría (descarga eléctrica) | segura | `BR00` :288, `NG00` :744 | 89 |
| 22 | `bhEne22` | en22.c:512 | Albinoid adulto | segura | `Brain` :1160 | 69 |
| 23 | `bhEne23` | en23.c:361 | Black Widow gigante (lleva crías) | probable | `Init` :447 | 147 |
| 24 | `bhEne24` | en24.c:76 | Cría de Black Widow | segura | `BR00` | 21 |
| 25 | `bhEne25` | en25.c:39 | Trampa de techo que aplasta (`plp->hp = -1`) | segura | `MV01` :171 | 21 |
| 26 | `bhEne26` | en26.c:628 | Zombi variante ("Anatomist Zombie") | probable | `Brain00` :1962 | 78 |
| 27 | `bhEne27` | en27.c:41 | Huevo y larva de polilla sobre el jugador (usa el modelo del jugador) | segura | `BR00` :146 | 64 |
| 28 | `bhEne28` | subpl.c:410 | Vacío (`{}`); no existe en28.c | — | — | 0 |
| 29 | `bhEne29` | en29.c:164 | Tentáculo fijo | probable | `Br00` :392, `Br01` :594 | 14 |
| 30 | `bhEne30` | en30.c:59 | Crías de Alexia | segura | — | 39 |
| 53 | `bhEne53` | en15.c:3726 | Corazón expuesto de Nosferatu | probable | `CoreInit` :3439, `CoreMove` :3472 | 0 |
| 54 | `bhEne54` | en54.c:45 | Armadura A (espada y escudo); cae con el hacha de Steve | segura | `CollCheck` :94 | 0 |
| 55 | `bhEne55` | en55.c:45 | Armadura B (lanza y escudo grande) | segura | `CollCheck` :94 | 0 |
| 56 | `bhEne56` | eneset.c:543 | Grúa de garra ("UFO catcher") que maneja el jugador | segura | `sys->ufo_*`; minijuego en `bhEff134` (effsub1b.c:221) | 0 |
| 71 | `bhEne71` | en71.c:23 | Insecto decorativo ("Ant C") | sin confirmar | — | 0 |

Casi todos los archivos siguen la misma estructura de máquina de estados: `bhEneNN_Mode0` → `BrainType` / `MoveMode2` / `NageMode2` / `DamageMode2` / `DeadMode2`.

### Archivos auxiliares

| Archivo | Qué contiene |
| --- | --- |
| en01b.c | La mitad "B" del zombi (un segundo actor id 1, probablemente el torso que se arrastra) y ayudas para el cuello. |
| en01sub.c | Hijos del zombi: cabeza (31), brazos (32), piernas (33), gorro (34), gusano (35), bomba (36), visor (37), coordinador de grupo (38). |
| en02sub.c | Partes del Gulp Worm: boca y 16 segmentos (id 31). |
| en03sub.c | Patas de araña cortadas que salen volando. Las usan en03 y en23. |
| en05sub.c | La cabeza del jugador decapitada por el Hunter. |
| en06sub.c | Los 14 trozos de la polilla al morir. |
| en13sub.c | El tentáculo de barrido de Alexia 2 (id 31). |
| en17sub.c | Los brazos de Steve monstruo (31 hacha, 32). |
| zonzon.c, zonzon1.c | Ayudas compartidas: giros, paredes, escaleras, sangre y efectos, cálculo de daño, esfera de ataque (`bhEne_SetWeponAtr`), `bhEne_AttackHitCheck`, sonido, `bhArcTan2`. |

### Quién crea o enlaza a quién

| Padre | Hijos y enlaces |
| --- | --- |
| en01 | id 1 (mitad B), ids 31-38 |
| en02 | 18× id 31 |
| en03 | 8× id 31, 15× id 24 |
| en05 | id 31 (cabeza) |
| en06 | 14× id 31; su agarre crea el id 27 |
| en13 | 10× id 18, id 31; enlaza los ids 30 y 14 |
| en15 | id 53 (corazón) |
| en17 | ids 31 y 32 (brazos) |
| en23 | enlaza los id 24 de la sala; 2× id 31 |
| en54 / en55 | reaccionan al id 17 (hacha de Steve) |

**Preguntas abiertas** (hay que mirar los datos de sala o probar en el juego): qué objeto es el id 0, si los ids 10/20/71 son hormigas, qué es el "Spotter" (id 11), dónde aparecen el id 29, la trampa 25 y la grúa 56, y qué zombi concreto es el id 26.

## NPCs de guion: `subpl.c`

- **Quiénes son:** Steve, Rodrigo, Alfred, Alexia, Wesker y Chris como NPC en las escenas.
- **Cómo funcionan:**
  - Son entradas de `ene[]` con ids 31-99 que despachan a `bhSubpl` (subpl.c:76): `flg |= 0x8100` → `Subpl_tbl[mode0]` (0 init, 1 mover, 5/6 escena) → `bhCalcModel`.
  - **No tienen IA, ni colisión con paredes, ni daño.**
  - Se mueven con primitivas de guion: `mv00_subpl0..10` (poner animación, andar a un punto empaquetado en `ct0`, girar). `bhSub_DirTarget` (subpl.c:376) da el giro limitado hacia un punto.
- **Cómo se controlan:** desde los scripts de evento (`bhLoadWork` caso 1, event.c:12739, y `Sub_controll`, event.c:9758).
- **Qué personaje es cada id:** solo está en los datos de sala (RDT), no en el código.
- **Esqueleto:** los ids 41-48 tienen un cuerpo de unos 20 huesos; los 91-98, el mismo cuerpo más 9 huesos de cara (`face.c`). **Ninguno usa el esqueleto de 22 huesos del jugador** (`PlyFlip`, player.c:219), así que no pueden usar sus animaciones.
- **Animaciones:** cada sala solo carga las que necesitan sus escenas.

Conclusión para un segundo jugador: no sirven como base. Ver [../coop/README.md](../coop/README.md).
