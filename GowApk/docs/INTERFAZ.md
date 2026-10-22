# Interfaz de GowApk

Este documento explica de donde salen las imagenes de menu de **GowApk** y como se
arma la pantalla de inicio.

El material viene del paquete original desempaquetado del juego chino
**战神之刃** (en el propio logo aparece el subtitulo en ingles **"GOD OF BLADE"**).
Ese paquete esta en `nuevosSprites` y es el mismo que ya usa el MODO BETA de
District Fury.

Nada se escalo, se recorto ni se redibujo: las imagenes se copiaron tal cual y
solo se les cambio el nombre al espanol.

- Archivos copiados: **131** en `GowApk/assets/ui/` (129 imagenes `.png` y 2
  descriptores `.plist` de los dos atlas de animacion del logo)
- Ademas se copio el heroe animado del menu (Spine 2.1) a `GowApk/assets/spine/menu/`
  sin tocar los nombres, porque el `.atlas` y el `.json` se llaman entre si:
  `zhujue.json`, `zhujue.atlas`, `zhujue.png`, `zhujue2.png`, `zhujue3.png`, `zhujue4.png`.
  Tiene **una sola animacion**, llamada `animation` (el heroe quieto, respirando).

> Nota de propiedad intelectual: el paquete es contenido de un juego de terceros.
> Igual que el MODO BETA, esto es una prueba interna y vive aislado.

---

## 1. Tabla de archivos copiados

El tamano es el original en pixeles. El lienzo del juego original era de **800x480**.

### Pantalla de carga (viene de `uidata/GameLoading.csb`)

| Archivo copiado | Archivo original | Para que sirve | En que pantalla va |
|---|---|---|---|
| `fondo_carga.png` (400x193) | `image/Bground/Bground_Loadingbackground.png` | fondo oscuro de nubes | Carga |
| `texto_loading.png` (155x36) | `image/Font/Font_Loading.png` | la palabra "Loading" dibujada | Carga |
| `texto_cargando.png` (238x34) | `image/Font/Font_Zhengzaijiazai.png` | texto "cargando..." en chino | Carga |
| `marco_barra_carga.png` (556x37) | `image/Image/Image_Loadingkuang.png` | marco vacio de la barra | Carga |
| `barra_carga.png` (528x17) | `image/Image/Image_Loadingtiao.png` | relleno de la barra | Carga |
| `brillo_barra_carga.png` (77x57) | `image/Image/Image_Loadingguangbiao.png` | chispa que corre sobre la barra | Carga |
| `ilustracion_carga_01.png` ... `ilustracion_carga_13.png` (450x272 aprox.) | `image/Image/Image_loadingtupian1..13.png` | 13 dibujos grandes que se van turnando mientras carga | Carga |
| `atlas_logo_arranque.png` (484x756) + `atlas_logo_arranque.plist` | `plist/plist/Effect_chouzhen.png` y `.plist` | hoja con los **20 cuadros** del logo en llamas de la empresa, que se anima al abrir el juego | Arranque (antes de la carga) |

### Logo y menu principal (viene de `uidata/GameMenu.csb`)

| Archivo copiado | Archivo original | Para que sirve | En que pantalla va |
|---|---|---|---|
| `logo.png` (800x480) | `image/Font/Font_logo.png` | el logo completo del juego, ya con su resplandor rojo | Menu principal |
| `logo_base.png` (456x250) | `image/Image/Image_logodi.png` | peana / base bajo el logo | Menu principal |
| `logo_fondo.png` (237x175) | `image/Image/Image_logoback.png` | mancha de fondo detras del logo | Menu principal |
| `logo_brillo.png` (788x109) | `image/Image/Image_logoguang.png` | barra de luz horizontal que cruza el logo | Menu principal |
| `atlas_brillo_logo.png` (1980x800) + `atlas_brillo_logo.plist` | `plist/plist/Logozhanshentexiao.png` y `.plist` | hoja con los **12 cuadros** del destello que recorre el logo del juego | Menu principal |
| `boton_jugar.png` (307x53) | `image/Button/Button_Kaishiyouxi.png` | boton "开始游戏" = EMPEZAR PARTIDA | Menu principal |
| `boton_sonido_on.png` (40x38) | `image/Button/Button_Yinliang.png` | altavoz encendido | Menu principal / Pausa |
| `boton_sonido_off.png` (40x38) | `image/Button/Button_Yinliang2.png` | altavoz apagado | Menu principal / Pausa |
| `boton_soporte.png` (109x41) | `image/Button/Button_Kefu.png` | boton de atencion al jugador | Menu principal |
| `boton_codigo.png` (139x47) | `image/Button/Button_Duihuanma.png` | boton de codigo de canje | Menu principal |
| `texto_mas_juegos.png` (92x28) | `image/Font/Font_Gengduoyouxi.png` | texto "mas juegos" | Menu principal |
| `texto_foro.png` (92x31) | `image/Font/Font_Luntan.png` | texto "foro" | Menu principal |
| `punto_rojo_aviso.png` (10x10) | `image/Image/Image_zhuchengzuobiaohongdian.png` | puntito rojo de aviso sobre un boton | Menu / Ciudad |

### Ciudad principal, el "hub" (viene de `uidata/GameHome.csb`)

| Archivo copiado | Archivo original | Para que sirve | En que pantalla va |
|---|---|---|---|
| `fondo_ciudad.png` (400x480) | `image/Image/Image_zhuchengchangjing.png` | templo griego con columnas y escalera | Menu principal (fondo) |
| `cielo_ciudad.png` (400x270) | `image/Image/Image_zhuchengtiankong.png` | cielo detras del templo | Menu principal (fondo) |
| `trono_ciudad.png` (61x57) | `image/Image/Image_zhuchengbaozuo.png` | trono donde se para el heroe | Menu principal |
| `sombra_ciudad.png` (99x36) | `image/Image/Image_zhuchengyingzi.png` | sombra bajo el heroe | Menu principal |
| `estatua_ciudad_01..05.png` | `image/Image/Image_zhuchengdiaoxiang01..05.png` | 5 estatuas decorativas del templo | Menu principal |
| `boton_combatir.png` (176x66) | `image/Button/Button_Chuzhan.png` | boton grande "salir a combatir" | Menu principal |
| `boton_combatir_brillo.png` (282x178) | `image/Image/Image_chuzhananniuguang.png` | halo que late sobre ese boton | Menu principal |
| `boton_armas.png` (76x73) | `image/Button/Button_Wuqi.png` | icono de armas | Menu principal |
| `boton_habilidades.png` (63x64) | `image/Button/Button_Jineng.png` | icono de habilidades | Menu principal |
| `boton_personaje.png` (52x84) | `image/Button/Button_Juese.png` | icono de personaje | Menu principal |
| `retrato_heroe.png` (61x64) | `image/Image/Image_Touxiang1.png` | retrato pequeno del heroe | Menu / Combate |
| `marco_barra_vida.png` (245x74) | `image/Image/Image_Xuetiaokuang.png` | marco del retrato y barras | Menu / Combate |
| `barra_vida.png` (85x8) | `image/Image/Image_Hongtiao.png` | barra roja de vida | Menu / Combate |
| `barra_magia.png` (82x8) | `image/Image/Image_Lantiao.png` | barra azul de magia | Menu / Combate |
| `barra_experiencia.png` (700x14) | `image/Image/Image_Jingyantiao.png` | barra de experiencia | Menu principal |
| `marco_experiencia_01.png` / `_02.png` (757x21) | `image/Image/Image_Jingyankuang1.png` / `2.png` | marco de la barra de experiencia | Menu principal |
| `fondo_nombre.png` (166x34) | `image/Image/Image_mingzichendi.png` | fondo oscuro bajo el nombre | Menu / Dialogos |
| `icono_moneda.png` (67x50) | `image/Icon/Icon_jinbi.png` | moneda de oro | Menu principal |
| `icono_alma_roja.png` (41x41) | `image/Icon/Icon_honghun.png` | "alma roja" (la otra moneda) | Menu principal |
| `aro_luz.png` (151x151) | `image/Image/Image_guangquan.png` | aro de luz decorativo | Menu / Seleccion |

### Seleccion de nivel (viene de `uidata/Xuanxiaoguan.csb` y `uidata/UIguanqiaxize.csb`)

| Archivo copiado | Archivo original | Para que sirve | En que pantalla va |
|---|---|---|---|
| `mapa_mundo_01.png` (800x480) | `image/Bground/Bground_BJ.png` | mapa 1: costa rocosa con camino punteado | Seleccion |
| `mapa_mundo_02_nieve.png` (800x480) | `image/Bground/Bground_BJxueshan.png` | mapa 2: montana nevada | Seleccion |
| `mapa_mundo_03.png` (800x480) | `image/Bground/Bground_Disanzhangditu.png` | mapa 3: tercer capitulo | Seleccion |
| `marco_mapa_mundo.png` (800x480) | `image/Bground/Bground_UIkuang.png` | marco de cadenas con el rotulo "世界地图" (MAPA DEL MUNDO) | Seleccion |
| `boton_nivel.png` (100x94) | `image/Button/Button_xuanguan.png` | escudo de un nivel abierto | Seleccion |
| `boton_nivel_bloqueado.png` (82x94) | `image/Button/Button_xuanguanhui.png` | el mismo escudo en gris (bloqueado) | Seleccion |
| `boton_volver.png` (67x66) | `image/Button/Button_fanhui.png` | flecha de volver | Seleccion / Pausa |
| `barra_progreso.png` (138x49) | `image/Image/Image_Jindutiao.png` | barra de avance del capitulo | Seleccion |
| `fondo_barra_progreso.png` (153x64) | `image/Image/Image_Jindutiaobeijing.png` | fondo de esa barra | Seleccion |
| `estrella_llena.png` (26x24) | `image/Icon/Icon_jinxing.png` | estrella ganada | Seleccion / Victoria |
| `estrella_vacia.png` (26x24) | `image/Icon/Icon_huixing.png` | estrella sin ganar | Seleccion / Victoria |
| `fondo_estrellas.png` (243x28) | `image/Image/Image_xingxingdi.png` | tira donde se apoyan las estrellas | Seleccion |
| `panel_detalle_nivel.png` (281x430) | `image/Bground/Bground_daxuanfuchuang.png` | ventana alta con detalles del nivel | Seleccion |
| `texto_detalle_nivel.png` (130x36) | `image/Font/Font_guanqiaxize.png` | titulo "detalle del nivel" | Seleccion |
| `boton_entrar_combate.png` (167x58) | `image/Button/Button_chuzhan2.png` | boton de entrar al nivel | Seleccion |
| `boton_azul.png` (166x51) | `image/Button/Button_lanseanniu.png` | boton azul secundario | Seleccion |
| `boton_cerrar.png` (47x48) | `image/Button/Button_tuichu.png` | X de cerrar ventana | todas las ventanas |
| `boton_grande.png` (183x67) | `image/Button/Button_wuzianniu2.png` | boton grande vacio (se le pone texto encima) | todas |
| `boton_mediano.png` (117x52) | `image/Button/Button_wuzianniu.png` | boton mediano vacio | todas |
| `icono_cofre.png` (71x56) | `image/Icon/Icon_baoxiang1.png` | cofre de recompensa | Seleccion |

### Pausa (viene de `uidata/GameSystem.csb`)

| Archivo copiado | Archivo original | Para que sirve | En que pantalla va |
|---|---|---|---|
| `panel_pausa.png` (283x103) | `image/Image/Image_Caidankuang1.png` | cabecera de madera del cuadro | Pausa |
| `base_pausa.png` (264x56) | `image/Image/Image_Zantingdituo.png` | pie del cuadro | Pausa |
| `cadena_pausa.png` (29x326) | `image/Image/Image_Tielian2.png` | cadena vertical que sujeta el cuadro | Pausa |
| `fila_pausa.png` (329x57) | `image/Button/Button_Zantingliebiao.png` | tira roja de cada opcion del menu | Pausa |
| `texto_pausa.png` (129x34) | `image/Font/Font_Youxizanting.png` | titulo "JUEGO EN PAUSA" | Pausa |
| `texto_volver_juego.png` (113x38) | `image/Font/Font_Fanhuiyouxi.png` | opcion "seguir jugando" | Pausa |
| `texto_volver_menu.png` (113x38) | `image/Font/Font_Huizhucaidan.png` | opcion "volver al menu" | Pausa |
| `texto_sonido_on.png` (114x39) | `image/Font/Font_Shengyinkaiqi.png` | opcion "sonido activado" | Pausa |
| `texto_sonido_off.png` (114x39) | `image/Font/Font_Shengyinguanbi.png` | opcion "sonido apagado" | Pausa |
| `texto_ayuda.png` (113x38) | `image/Font/Font_Youxibangzhu.png` | opcion "ayuda" | Pausa |
| `icono_sonido.png` / `icono_sonido_apagado.png` (41x41) | `image/Font/Font_Shengyintubiao.png` / `2.png` | altavoz encendido / tachado | Pausa |
| `boton_pausa.png` (38x37) | `image/Button/Button_zhanting.png` | boton de pausa que va en el combate | Combate |

### Victoria, derrota y preparacion (`Zhandoushengli.csb`, `Zhandoushibai.csb`, `Zhandouzhunbei.csb`)

| Archivo copiado | Archivo original | Para que sirve | En que pantalla va |
|---|---|---|---|
| `texto_victoria.png` (130x36) | `image/Font/Font_zhandoushengli.png` | rotulo "VICTORIA" | Victoria |
| `texto_derrota.png` (130x36) | `image/Font/Font_zhandoushibai.png` | rotulo "DERROTA" | Derrota |
| `texto_preparar_combate.png` (130x36) | `image/Font/Font_zhandouzhunbei.png` | rotulo "preparacion de combate" | Antes del nivel |
| `fondo_resultado.png` (281x352) | `image/Image/Image_beijing.png` | panel del resumen | Victoria |
| `cabecera_resultado.png` (165x126) | `image/Image/Image_biaotitou.png` | adorno de cabecera | Victoria / Derrota |
| `cabecera_izquierda.png` (172x95) | `image/Image/Image_biaotizuo.png` | adorno de cabecera (lado izquierdo) | Victoria / Derrota |
| `fondo_recompensa.png` (58x49) | `image/Image/Image_Lingqudikuang.png` | casilla de un premio | Victoria |
| `fondo_resumen.png` (419x196) | `image/Image/Image_Wanchengqingkuangdi.png` | tabla con lo conseguido | Victoria |
| `texto_siguiente_nivel.png` (86x54) | `image/Font/Font_xiayiguan.png` | boton "siguiente nivel" | Victoria |
| `texto_volver_ciudad.png` (89x56) | `image/Font/Font_huizhucheng.png` | boton "volver a la ciudad" | Victoria / Derrota |
| `texto_tiempo_nivel.png` (83x24) | `image/Font/Font_guanqiayongshi.png` | "tiempo usado en el nivel" | Victoria |
| `icono_check.png` (64x54) | `image/Icon/Icon_duigou.png` | objetivo cumplido | Victoria |
| `icono_cruz.png` (54x48) | `image/Icon/Icon_chahao.png` | objetivo fallado | Victoria |
| `destello_barra.png` (547x248) | `image/Image/Image_shanguangtiao.png` | destello de la animacion de victoria | Victoria |

### Dialogos de historia (viene de `uidata/Dilog.csb`)

| Archivo copiado | Archivo original | Para que sirve | En que pantalla va |
|---|---|---|---|
| `marco_dialogo.png` (448x142) | `image/Image/Image_duihuakuang.png` | cuadro verde-dorado donde va el texto | Dialogo |
| `retrato_guia.png` (234x257) | `image/Image/Image_Wudini.png` | chica rubia que hace de guia | Dialogo |
| `retrato_kratos.png` (393x328) | `image/Image/Image_Kuituosi.png` | el heroe principal | Dialogo |
| `retrato_poseidon.png` (383x300) | `image/Image/Image_Bosaidong.png` | Poseidon | Dialogo |
| `retrato_medusa.png` (418x326) | `image/Image/Image_meidusha.png` | Medusa | Dialogo |
| `retrato_titan.png` (568x300) | `image/Image/Image_Taitanjuren.png` | Titan | Dialogo |
| `retrato_atenea.png` (361x346) | `image/Image/Image_Yadianna.png` | Atenea | Dialogo |
| `retrato_hidra.png` (510x358) | `image/Image/Image_Xudela.png` | Hidra | Dialogo |
| `retrato_centauro.png` (446x286) | `image/Image/Image_Renma.png` | Centauro | Dialogo |
| `retrato_monstruo.png` (311x296) | `image/Image/Image_Guaiwu.png` | monstruo generico | Dialogo |
| `retrato_bruto_cadena.png` (453x290) | `image/Image/Image_lianqiuguai.png` | bruto de la bola y cadena | Dialogo |

### Ventana de confirmacion y piezas comunes

| Archivo copiado | Archivo original | Para que sirve | En que pantalla va |
|---|---|---|---|
| `panel_confirmacion.png` (319x236) | `image/Bground/Bground_xiaoxuanfuchuang.png` | ventanita de "¿seguro?" | Confirmacion |
| `texto_aviso.png` (74x24) | `image/Font/Font_querentishi.png` | titulo "aviso" | Confirmacion |
| `texto_confirmar.png` (105x92) | `image/Font/Font_queding.png` | "aceptar" | Confirmacion |
| `texto_cancelar.png` (67x42) | `image/Font/Font_quxiao.png` | "cancelar" | Confirmacion |
| `texto_confirmar_salir.png` (239x56) | `image/Font/Font_querentuichuwenzi.png` | "¿seguro que quieres salir?" | Confirmacion |
| `fondo_comun.png` (800x480) | `image/Bground/Bground_Tongyong.png` | fondo generico de ventana grande | varias |
| `fondo_tienda.png` (800x423) | `image/Bground/Bground_shangchengditu.png` | fondo de la preparacion / tienda | Preparacion |
| `marco_ui.png` (800x480) | `image/Bground/Frame_UIkuang.png` | marco de pantalla completa sin rotulo | varias |
| `fondo_texto.png` (192x42) | `image/Bground/Bground_wenzidikuang.png` | tira oscura para poner texto | varias |
| `degradado_negro.png` (335x184) | `image/Image/Image_Heidijianbian.png` | degradado negro para oscurecer | Pausa / Dialogo |
| `selector.png` (489x68) | `image/Image/Image_Xuanzekuang.png` | marco de la fila elegida en una lista | varias |
| `estandarte_heroe.png` (135x281) | `image/Image/Image_hongsezhanshen.png` | estandarte rojo con el heroe | varias |
| `arte_heroe.png` (345x175) | `image/Image/Image_zhanshen.png` | dibujo del heroe con las espadas encendidas | Menu / Victoria |

---

## 2. Como era el arranque del juego original

Esto sale de leer el paquete (los `.csb` y los `.lua`), no de suposiciones.

**Paso 1 - Logo de la empresa.**
`uidata/UITloading.csb` es una animacion de 20 cuadros
(`image/Effect/Effect_chouzhen/logo1..20.png`). La carpeta `image/Effect/` **no vino**
en el paquete, **pero los 20 cuadros si estan**, empaquetados en el atlas
`plist/plist/Effect_chouzhen.png` + `.plist`. Es el emblema de la empresa ardiendo en
llamas naranjas. Se copio como `atlas_logo_arranque.png` / `.plist` (el `.plist` se
edito solo para que apunte al nuevo nombre del `.png`; los recortes no se tocaron).

**Paso 2 - Pantalla de carga.**
`uidata/GameLoading.csb` arma esto: el fondo oscuro de nubes
(`Bground_Loadingbackground.png`), un dibujo grande que va cambiando
(`Image_loadingtupian1..13.png`, trece ilustraciones distintas), la palabra
"Loading", y abajo la barra: marco + relleno + una chispa que corre encima.
`data/preLoadingCSBFile.lua` dice que mientras tanto se precargan 22 pantallas
(ciudad, combate, QTE, habilidades, mapa del mundo, victoria, derrota, etc.).

**Paso 3 - Menu principal (pantalla de titulo).**
`uidata/GameMenu.csb`. Es muy simple: el **logo** del juego (`Font_logo.png`,
una imagen de 800x480 ya con el resplandor rojo pintado; dice 战神之刃 y debajo,
en letras pequenas, **GOD OF BLADE**), su base y su halo, una barra de luz que lo
cruza, y el boton grande **开始游戏 = EMPEZAR PARTIDA**. A los lados, botones
chicos: sonido on/off, atencion al jugador, codigo de canje, "foro" y "mas juegos".
Encima se reproduce `uidata/UITzhanshenlogotexiao.csb`, un efecto de 12 cuadros
sobre el logo (`image/Effect/Effect_Logozhanshentexiao/lo_00..11.png`). Esos cuadros
tambien estan en un atlas: `plist/plist/Logozhanshentexiao.png` + `.plist`, copiado
como `atlas_brillo_logo.png` / `.plist`.

**Paso 4 - Ciudad principal (el "hub").**
`uidata/GameHome.csb`. Al tocar EMPEZAR PARTIDA se llega aqui. Es un templo griego
(`Image_zhuchengchangjing.png` + cielo + 5 estatuas), con el heroe parado sobre un
trono en el centro. El heroe **no es un dibujo fijo**: es el esqueleto Spine
`spine/Menu/zhujue.json`, con una sola animacion en bucle. Arriba van el retrato,
la vida, la magia y la experiencia; a la derecha monedas y almas rojas; abajo los
botones de armas, habilidades y personaje; y el boton grande de salir a combatir,
con un halo que late.

**Paso 5 - Mapa del mundo / seleccion de nivel.**
`uidata/Xuanxiaoguan.csb`. Un mapa pintado a mano de 800x480 con un camino
punteado, y escudos redondos en cada parada: dorado si esta abierto
(`Button_xuanguan.png`), gris si esta bloqueado (`Button_xuanguanhui.png`).
Encima va el marco de cadenas con el rotulo "世界地图" (MAPA DEL MUNDO). Hay tres
mapas: costa rocosa, montana nevada y un tercer capitulo. Al elegir una parada se
abre la ventana de detalle (`UIguanqiaxize.csb`) con estrellas, recompensas y el
boton de entrar.

**Paso 6 - Dialogo de historia.**
`uidata/Dilog.csb`. Un cuadro verde-dorado abajo, el retrato del que habla a un
lado, y su nombre sobre una tira oscura. Los textos estan en `data/Dialog.lua`
(en chino) y los guiones por nivel en `story/*.lua`.

**Paso 7 - Combate, pausa, victoria y derrota.**
En combate, el boton de pausa (`Button_zhanting.png`) abre `GameSystem.csb`: un
cuadro de madera colgado de cadenas con cuatro filas rojas: seguir jugando,
sonido, ayuda y volver al menu. Al terminar: `Zhandoushengli.csb` (VICTORIA, con
estrellas y premios) o `Zhandoushibai.csb` (DERROTA, que ofrece mejorar el arma y
reintentar).

### Que musica suena (de `data/GameMusicFile.lua`)

La funcion `GameUIBackRoundMusic(UIType)` del juego original dice exactamente esto:

| Momento | Archivo | Duracion aprox. |
|---|---|---|
| Menu principal (pantalla de titulo) | `sound/menu.ogg` | el mas largo de los de menu (273 KB) |
| Ciudad principal y pantallas de armas, habilidades, torre y ranking | `sound/music_001.ogg` | 218 KB |
| Escena de historia / CG | `sound/gameCG.ogg` | 114 KB |
| Victoria | `sound/music_002.ogg` | 55 KB (corto, es un remate) |
| Derrota | `sound/music_003.ogg` | 44 KB (corto) |
| Dentro del nivel, tipo barco | `sound/gate1music.ogg` | 203 KB |
| Dentro del nivel, tipo volcan | `sound/gate2music.ogg` | 189 KB |

Sonidos sueltos utiles para los menus: `sound1` = abrir ventana,
`sound2` = cerrar ventana, `sound13` = cambiar habilidad, `sound14` = cambiar arma.

**La pantalla de carga no tiene musica propia** en esa tabla: no hay ninguna
entrada para ella.

---

## 3. Que se vio en internet

Se busco varias veces (en chino y en ingles). Resultado honesto: **solo una fuente
util**, y ninguna captura de la pantalla de titulo.

1. **9game.cn, ficha de descarga de 战神之刃** - https://www.9game.cn/xiazai/730260/
   Es la unica pagina que corresponde de verdad al juego del paquete. Dice que es un
   juego de accion para un jugador de **北京元火科技 (Beijing Yuanhuo Technology)**,
   con **tres mapas del mundo** distintos, mecanismos y cofres en los niveles,
   misiones de escolta y de proteger estatuas, **cuatro armas legendarias** (las tres
   primeras se consiguen juntando fragmentos), **QTE contra jefes gigantes**, y
   sistemas de "barrer" niveles, monedas, **almas rojas** y **torre de mil pisos**.
   Todo eso coincide pieza por pieza con el paquete (`Qiancengta`, `saodang`,
   `honghun`, `GameQTE`, cuatro armas). La pagina tiene una zona de capturas, pero
   solo con texto alternativo generico: **no se pudo ver ninguna imagen**.
2. **La misma ficha en 9game, otra version** - https://www.9game.cn/xiazai/1304629/
   Es otro juego distinto ("战神之刃传奇"), no sirve.
3. **Moegirl, articulo "混沌之刃"** - https://zh.moegirl.org.cn/混沌之刃
   Habla de las Espadas del Caos del God of War original de Sony. Sirve solo para
   confirmar de donde copiaron el arma, no para ver el menu.
4. **Ficha de App Store** - https://apps.apple.com/cn/app/id930389194 - no es el
   mismo juego.

**No se encontro**: ninguna captura ni video del arranque, del menu principal o de
la intro de este juego. Tampoco su fecha original de salida fiable (la ficha de
9game muestra una version y una fecha actualizadas, no las de 2015-2016, que es la
fecha que aparece dentro de los `.lua` del paquete). Lo que se describe en la
seccion 2 sale **solo del paquete**, que es informacion de primera mano y segura.

---

## 4. Propuesta de 4 pantallas para GowApk (lienzo 1280x720)

El arte original esta hecho para 800x480. En 1280x720 **no hay que escalar las
imagenes**: se colocan a tamano real y se usa el espacio de sobra para separar
mejor los elementos. Las coordenadas de abajo son de la **esquina superior
izquierda** de cada imagen, salvo donde diga "centrado".

### Pantalla 1 - CARGA

Antes de esta pantalla va, opcionalmente, el logo de la empresa en llamas:
los 20 cuadros de `atlas_logo_arranque` centrados en el lienzo, a unos 12 cuadros
por segundo, sobre fondo negro, una sola vez.

| Pieza | Imagen | Posicion | Notas |
|---|---|---|---|
| Fondo | `fondo_carga.png` (400x193) | repetido en mosaico sobre todo el lienzo | es una textura de nubes oscuras, se repite bien |
| Ilustracion | `ilustracion_carga_01..13.png` (450x272) | centrada en x=415, y=170 | se elige una al azar en cada carga |
| Marco de barra | `marco_barra_carga.png` (556x37) | x=362, y=560 | centrado horizontalmente |
| Relleno de barra | `barra_carga.png` (528x17) | x=376, y=570 | se recorta por la derecha segun el avance (0 a 528 px) |
| Chispa | `brillo_barra_carga.png` (77x57) | y=550, x = 376 + avance - 38 | sigue la punta del relleno |
| Texto | `texto_loading.png` (155x36) | x=562, y=610 | centrado |
| Sin musica | - | - | el original no pone musica aqui |

### Pantalla 2 - MENU PRINCIPAL

| Pieza | Imagen | Posicion | Notas |
|---|---|---|---|
| Cielo | `cielo_ciudad.png` (400x270) | x=0, y=0, repetido en x hasta cubrir 1280 | |
| Templo | `fondo_ciudad.png` (400x480) | x=440, y=240 | centrado abajo |
| Estatuas | `estatua_ciudad_01..05.png` | 01 en x=120 y=300; 02 en x=300 y=270; 04 en x=820 y=250; 05 en x=1010 y=290 | decoran los lados |
| Trono | `trono_ciudad.png` (61x57) | x=610, y=600 | justo bajo el heroe |
| Sombra | `sombra_ciudad.png` (99x36) | x=590, y=630 | |
| Heroe animado | `spine/menu/zhujue.json` | pies en x=640, y=640 | animacion `animation` en bucle |
| Halo del logo | `logo_fondo.png` (237x175) | x=521, y=20 | detras del logo |
| Base del logo | `logo_base.png` (456x250) | x=412, y=10 | |
| Logo | `logo.png` (800x480) | x=240, y=-110 | la imagen ya trae su resplandor; sobresale por arriba a proposito |
| Barra de luz | `logo_brillo.png` (788x109) | x=246, y=150 | se mueve en bucle de izquierda a derecha |
| Boton jugar | `boton_jugar.png` (307x53) | x=486, y=560 | centrado; late despacio |
| Halo del boton | `boton_combatir_brillo.png` (282x178) | x=499, y=498 | detras del boton |
| Sonido | `boton_sonido_on.png` / `_off.png` (40x38) | x=1220, y=20 | esquina superior derecha |
| Soporte | `boton_soporte.png` (109x41) | x=20, y=660 | esquina inferior izquierda |
| Codigo | `boton_codigo.png` (139x47) | x=145, y=657 | al lado del anterior |
| Foro | `texto_foro.png` (92x31) | x=1050, y=665 | |
| Mas juegos | `texto_mas_juegos.png` (92x28) | x=1160, y=667 | |
| Musica | `sound/menu.ogg` | en bucle | |

### Pantalla 3 - SELECCION DE NIVEL

| Pieza | Imagen | Posicion | Notas |
|---|---|---|---|
| Mapa | `mapa_mundo_01.png` (800x480) | x=240, y=120 | centrado; se cambia por `_02_nieve` o `_03` segun el capitulo |
| Marco | `marco_mapa_mundo.png` (800x480) | x=240, y=120 | va justo encima del mapa, ya trae el rotulo del titulo |
| Escudos de nivel | `boton_nivel.png` (100x94) | sobre el camino punteado del mapa, sumando 240 y 120 a la posicion que tengan en el mapa | el bloqueado usa `boton_nivel_bloqueado.png` |
| Estrellas de cada nivel | `estrella_llena.png` / `estrella_vacia.png` (26x24) | tres seguidas, 2 px de separacion, 10 px bajo cada escudo | |
| Tira de estrellas | `fondo_estrellas.png` (243x28) | x=518, y=612 | resumen del capitulo, centrado |
| Barra de avance | `fondo_barra_progreso.png` (153x64) + `barra_progreso.png` (138x49) | fondo en x=1060 y=30; relleno en x=1068 y=38 | |
| Volver | `boton_volver.png` (67x66) | x=30, y=30 | esquina superior izquierda |
| Panel de detalle | `panel_detalle_nivel.png` (281x430) | x=500, y=145 | solo al elegir un nivel; aparece centrado |
| Titulo del panel | `texto_detalle_nivel.png` (130x36) | x=575, y=165 | |
| Entrar | `boton_entrar_combate.png` (167x58) | x=557, y=500 | dentro del panel |
| Cerrar | `boton_cerrar.png` (47x48) | x=740, y=150 | esquina del panel |
| Cofre | `icono_cofre.png` (71x56) | x=530, y=400 | premio del nivel |
| Musica | `sound/music_001.ogg` | en bucle | es la de los menus de la ciudad |

### Pantalla 4 - PAUSA (encima del combate)

| Pieza | Imagen | Posicion | Notas |
|---|---|---|---|
| Oscurecido | `degradado_negro.png` (335x184) | estirado sobre todo el lienzo, o un rectangulo negro al 60% | para apagar el combate de fondo |
| Cadena izquierda | `cadena_pausa.png` (29x326) | x=455, y=0 | cuelga desde arriba |
| Cadena derecha | `cadena_pausa.png` (29x326) | x=796, y=0 | |
| Cabecera | `panel_pausa.png` (283x103) | x=498, y=130 | centrada |
| Titulo | `texto_pausa.png` (129x34) | x=575, y=163 | sobre la cabecera |
| Fila 1 | `fila_pausa.png` (329x57) + `texto_volver_juego.png` | fila en x=475, y=250; texto centrado dentro | seguir jugando |
| Fila 2 | `fila_pausa.png` + `texto_sonido_on.png` / `_off.png` | x=475, y=317 | con `icono_sonido.png` en x=505, y=326 |
| Fila 3 | `fila_pausa.png` + `texto_ayuda.png` | x=475, y=384 | |
| Fila 4 | `fila_pausa.png` + `texto_volver_menu.png` | x=475, y=451 | abre la ventana de confirmacion |
| Pie | `base_pausa.png` (264x56) | x=508, y=520 | cierra el cuadro |
| Confirmacion | `panel_confirmacion.png` (319x236) + `texto_confirmar_salir.png` + `texto_confirmar.png` + `texto_cancelar.png` | panel en x=480, y=242 | solo al salir al menu |
| Boton de pausa en combate | `boton_pausa.png` (38x37) | x=1225, y=15 | es el que abre esta pantalla |

Ancho del cuadro de pausa: las cadenas quedan justo a los lados de la cabecera
(498 - 29 = 469, y 498 + 283 = 781), por eso x=455 y x=796 con unos pocos px de aire.

---

## 5. Lo que NO se encontro

Dicho claro, para que nadie lo busque dos veces:

1. **Los PNG sueltos de las animaciones del logo.** La carpeta
   `image/Effect/` no existe en el paquete, asi que no hay un archivo por cuadro.
   Los cuadros **si** se recuperaron, pero vienen pegados en dos hojas (atlas) con
   un `.plist` que dice donde esta cada uno:
   `atlas_logo_arranque` (20 cuadros) y `atlas_brillo_logo` (12 cuadros).
   Para usarlos hay que leer el `.plist` y recortar; ojo, algunos recortes estan
   **girados 90 grados** (`rotated`), como es normal en los atlas de cocos2d.
2. **Capturas reales del juego original.** No hay ninguna imagen del menu o de la
   intro en internet que se haya podido ver (ver seccion 3).
3. **Fondo propio del menu principal.** `GameMenu.csb` **no carga ningun fondo**:
   solo logo y botones. En el juego real el fondo probablemente era un color plano
   o venia del efecto de arranque que falta. La propuesta de arriba usa el templo
   de la ciudad (`GameHome.csb`) como fondo porque queda bien y es del mismo juego,
   pero **eso es una decision nuestra, no una copia de lo original**.
4. **Coordenadas exactas de lo original.** Los `.csb` son binarios de cocos2d y
   aqui solo se leyeron sus cadenas de texto para saber **que imagen usa cada
   pantalla**. Las posiciones de la seccion 4 son una propuesta nueva para
   1280x720, no las medidas originales.
5. **Posicion de los escudos sobre el mapa del mundo.** Estan dentro del `.csb` de
   `Xuanxiaoguan` como coordenadas binarias; no se extrajeron. Hay que colocarlos
   a mano siguiendo el camino punteado del mapa.
6. **Pantalla de pausa con imagen de fondo propia.** `GameSystem.csb` tampoco
   trae fondo: se dibuja encima del combate.
7. **Fuentes de letra.** `Dilog.csb` pide `font/heiti.ttf` y `StoryDarkScreen.csb`
   pide `font/lishu.ttf`. **La carpeta `font/` no vino en el paquete.** Todos los
   rotulos de menu que si tenemos son imagenes ya dibujadas (los `Font_*.png`), y
   estan **en chino**: si GowApk los quiere en espanol, habra que redibujarlos o
   escribir el texto con una fuente propia.
8. **Textos en espanol.** Todo el texto del paquete (`data/Dialog.lua`,
   `story/*.lua`, `data/CommonString.lua`) esta en chino y no se tradujo aqui.
