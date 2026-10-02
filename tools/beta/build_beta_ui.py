"""BETA: copia la interfaz original de batalla de nuevosSprites (image/) a
assets/beta/ui/ con nombres en espanol. Uso: python3 tools/beta/build_beta_ui.py <nuevosSprites>"""
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
UI = {
    # barra de vida del heroe (marco en forma de espada), retrato, vida y magia
    "barra_espada": "Image/Image_Xuetiaokuang.png", "retrato_guerrero": "Image/Image_Touxiang1.png",
    "barra_roja": "Image/Image_Hongtiao.png", "barra_azul": "Image/Image_Lantiao.png",
    # barra del jefe (marco + 3 capas: verde, amarilla, roja) y destello
    "jefe_marco": "Image/Image_Bossxuecao.png", "jefe_rojo": "Image/Image_Bossxuecaotianchong01.png",
    "jefe_amarillo": "Image/Image_Bossxuecaotianchong02.png", "jefe_verde": "Image/Image_Bossxuecaotianchong03.png",
    "jefe_destello": "Daji_shuzi/bossBloodEffect.png", "jefe_titulo": "Font/Font_Bosszhan.png",
    # barra de los enemigos
    "enemigo_fondo": "Daji_shuzi/bloodGroove.png", "enemigo_vida": "Daji_shuzi/bloodBar.png",
    # controles
    "joystick": "Button/Button_Xuniyaogan11.png", "flecha": "Button/Button_jiantou.png",
    "boton_ataque": "Button/Button_Xunianjian5.png", "boton_omega": "Button/Button_Xunianjian1.png",
    "boton_hab1": "Button/Button_Xunianjian2.png", "boton_hab2": "Button/Button_Xunianjian3.png",
    "boton_hab3": "Button/Button_Xunianjian4.png", "boton_qte": "Image/Image_QTEanniu.png",
    "pausa": "Button/Button_zhanting.png", "pocion_roja": "Icon/Icon_xueping.png", "pocion_azul": "Icon/Icon_lanping.png",
    "pociones_fondo": "Image/Image_yaopingdi.png",
    # armas (boton de cambio de arma)
    "arma_0": "Icon/Icon_hundun (1).png", "arma_1": "Icon/Icon_quantao.png",
    "arma_2": "Image/Image_Dianguanglian.png", "arma_3": "Icon/Icon_gouzhua.png",
    # numeros y combo
    "numeros_golpe": "Daji_shuzi/Dajishuzi.png", "numeros_critico": "Daji_shuzi/critNumber.png",
    "numeros_dano": "Daji_shuzi/attnum.png", "numeros_x": "Font/Font_0-9huangdazi.png", "hits": "Font/Font_HITS.png",
    "sangre_combo": "Image/Image_Xue.png", "guerrero_rojo": "Image/Image_hongsezhanshen.png",
    "guerrero": "Image/Image_zhanshen.png",
}

if __name__ == "__main__":
    src = Path(sys.argv[1]) / "image"
    dst = ROOT / "assets/beta/ui"
    dst.mkdir(parents=True, exist_ok=True)
    for name, rel in UI.items():
        shutil.copyfile(src / rel, dst / f"{name}.png")
    print(len(UI), "imagenes de interfaz en", dst)
