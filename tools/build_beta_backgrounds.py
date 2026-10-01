#!/usr/bin/env python3
"""Fondo BETA (assets/backgrounds/source/beta_vs.jpg, del usuario).

- assets/backgrounds/hd/vs_beta.png: escenario del Modo VS (sin texto extra).
- Copias BETA con codigo en neon (E<stage>.S<escenario>) en assets/backgrounds/beta/.
  NO se usan en el juego: los 20 escenarios originales (stageN_scenarioMM.png)
  siguen siendo los fondos. El BETA es solo para un escenario que no exista.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets/backgrounds/source/beta_vs.jpg"
HD = ROOT / "assets/backgrounds/hd"
BETA_DIR = ROOT / "assets/backgrounds/beta"
W, H = 1600, 720
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
MARK = "BETA"   # los archivos generados llevan esta marca en su metadato


def base():
    im = Image.open(SRC).convert("RGB")
    k = W / im.width
    im = im.resize((W, round(im.height * k)), Image.LANCZOS)
    top = max(0, min(im.height - H, round(120 * k / 1.5625)))
    return im.crop((0, top, W, top + H))


def neon(img, text, center):
    font = ImageFont.truetype(FONT, 64)
    glow = Image.new("RGBA", img.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(glow)
    w = d.textlength(text, font=font)
    x, y = center[0] - w / 2, center[1] - 32
    d.rounded_rectangle((x - 22, y - 10, x + w + 22, y + 78), 10, outline=(60, 140, 255, 255), width=5)
    d.text((x, y), text, font=font, fill=(90, 170, 255, 255))
    blur = glow.filter(ImageFilter.GaussianBlur(9))
    out = img.convert("RGBA")
    out.alpha_composite(blur); out.alpha_composite(blur)
    sharp = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ds = ImageDraw.Draw(sharp)
    ds.rounded_rectangle((x - 22, y - 10, x + w + 22, y + 78), 10, outline=(170, 215, 255, 255), width=2)
    ds.text((x, y), text, font=font, fill=(205, 232, 255, 255))
    out.alpha_composite(sharp)
    return out.convert("RGB")


def main():
    HD.mkdir(parents=True, exist_ok=True)
    b = base()
    b.save(HD / "vs_beta.png")
    made = []
    for st in range(1, 6):
        for sc in range(1, 5):
            if (HD / f"stage{st}_scenario{sc:02d}.png").exists(): continue   # fondo HD real
            BETA_DIR.mkdir(parents=True, exist_ok=True)
            f = BETA_DIR / f"stage{st}_scenario{sc:02d}.png"   # aparte: el juego usa el original
            neon(b, f"E{st}.S{sc}", (1120, 245)).save(f, pnginfo=_meta())
            made.append(f.name)
    print("vs_beta.png +", len(made), "fondos BETA:", ", ".join(made))


def _meta():
    from PIL.PngImagePlugin import PngInfo
    m = PngInfo(); m.add_text("df", MARK); return m


if __name__ == "__main__":
    main()
