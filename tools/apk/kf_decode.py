#!/usr/bin/env python3
"""Decodificador de animation.bin (APK King Fighter III, respaldo en
apk_reference/king_fighter_iii/bin). HERRAMIENTA DE ESTUDIO.

Formato (obtenido del codigo del juego: game.CGame / game.x / game.ba):
  cabecera : short nSprites(35), short nImagenes(213), (nSprites+1) int offsets
  sprite x : imagenes usadas, paletas alternativas, MODULOS (pieza = recorte de
             una imagen), FRAMES (lista de piezas con x,y,transformacion),
             ACCIONES (secuencia de frames con duracion en ticks y banderas),
             cajas de golpe (tipo 1) y de cuerpo (tipo 2) por frame.
  imagen ba: recortes (w,h,x,y), paletas PLTE opcionales y un PNG.
Transformaciones internas -> MIDP: [0,6,3,5,2,1,7,4].
"""
import io, json, struct, sys, zlib
from pathlib import Path
from PIL import Image

MIDP = [0, 6, 3, 5, 2, 1, 7, 4]   # interno -> MIDP transform


class R:
    def __init__(s, d, p=0): s.d, s.p = d, p
    def b(s): v = s.d[s.p]; s.p += 1; return v - 256 if v > 127 else v
    def ub(s): v = s.d[s.p]; s.p += 1; return v
    def h(s): v = struct.unpack_from('>h', s.d, s.p)[0]; s.p += 2; return v
    def i(s): v = struct.unpack_from('>i', s.d, s.p)[0]; s.p += 4; return v
    def raw(s, n): v = s.d[s.p:s.p + n]; s.p += n; return v


def parse_sprite(r):
    sp = {}
    n = r.h()
    sp['images'] = [r.h() for _ in range(n)]
    sp['img_lists'] = [[r.h() for _ in range(r.h())] for _ in range(n)]
    sp['alt_images'] = [[r.h() for _ in range(r.ub())] for _ in range(n)]
    sp['modules'] = [r.i() for _ in range(r.h())]            # (imgId<<16)|clip
    nf = r.h()
    boxidx = r.raw(nf * 2)                                   # por frame: caja1, caja2
    fcount = r.raw(nf)                                       # piezas por frame
    fstart = [r.h() for _ in range(nf + 1)]
    sp['frame_pieces'] = [r.i() for _ in range(r.h())]
    sp['frames'] = [{'start': fstart[k], 'count': fcount[k], 'hit': boxidx[2 * k], 'body': boxidx[2 * k + 1]} for k in range(nf)]
    na = r.h()
    act_prop = list(r.raw(na)); act_len = list(r.raw(na))
    astart = [r.h() for _ in range(na + 1)]
    nh = r.h(); h = [r.h() & 0xFFFF for _ in range(nh * 2)]
    ng = r.h(); nb = r.h()
    hit = [r.h() for _ in range(ng * 4)]; body = [r.h() for _ in range(nb * 4)]
    ne = r.h(); props = list(r.raw(ne * 5))
    acts = []
    for a in range(na):
        steps = []
        for k in range(act_len[a]):
            w0, w1 = h[astart[a] + 2 * k], h[astart[a] + 2 * k + 1]
            steps.append({'frame': w0 & 1023, 'ticks': (w0 >> 10) & 31, 'flags': w1})
        acts.append({'prop': act_prop[a], 'steps': steps, 'props5': props[act_prop[a] * 5: act_prop[a] * 5 + 5]})
    sp['actions'] = acts
    sp['hitboxes'] = [hit[k:k + 4] for k in range(0, len(hit), 4)]
    sp['bodyboxes'] = [body[k:k + 4] for k in range(0, len(body), 4)]
    return sp


def parse_image(r):
    has_clips = r.b() == 0
    nclip = r.h()
    clips = []
    if has_clips:
        wh = [(r.h(), r.h()) for _ in range(nclip)]
        xy = [(r.h(), r.h()) for _ in range(nclip)]
        clips = [(x, y, w, hh) for (w, hh), (x, y) in zip(wh, xy)]
    npal = r.b(); palsize = r.h()
    pals = []
    if palsize > 0:
        blob = r.raw(npal * palsize)
        pals = [blob[k * palsize:(k + 1) * palsize] for k in range(npal)]
    png = r.raw(r.i())
    return {'clips': clips, 'palettes': pals, 'png': png}


def png_with_palette(png, pal):
    """Reemplaza el chunk PLTE por otra paleta (asi recolorea el propio juego)."""
    out = bytearray(png[:8]); p = 8
    while p < len(png):
        n = struct.unpack('>I', png[p:p + 4])[0]; t = png[p + 4:p + 8]
        data = png[p + 8:p + 8 + n]
        if t == b'PLTE' and len(pal) == n: data = pal
        out += struct.pack('>I', len(data)) + t + data + struct.pack('>I', zlib.crc32(t + data) & 0xffffffff)
        p += 12 + n
    return bytes(out)


def load(path):
    d = Path(path).read_bytes(); r = R(d)
    ns, ni = r.h(), r.h()
    offs = [r.i() for _ in range(ns + 1)]
    base = r.p
    sprites = []
    for k in range(ns):
        r.p = base + offs[k]; sprites.append(parse_sprite(r))
    r.p = base + offs[ns]
    ioffs = [r.i() for _ in range(ni + 1)]
    ibase = r.p
    images = []
    for k in range(ni):
        r.p = ibase + ioffs[k]; images.append(parse_image(r))
    return sprites, images


def transform(img, midp):
    # MIDP: 0 NONE, 1 MIRROR_ROT180, 2 MIRROR, 3 ROT180, 4 MIRROR_ROT270, 5 ROT90, 6 ROT270, 7 MIRROR_ROT90
    T = Image.Transpose
    if midp == 0: return img
    if midp == 2: return img.transpose(T.FLIP_LEFT_RIGHT)
    if midp == 3: return img.transpose(T.ROTATE_180)
    if midp == 1: return img.transpose(T.FLIP_TOP_BOTTOM)
    if midp == 5: return img.transpose(T.ROTATE_270)          # 90 horario
    if midp == 6: return img.transpose(T.ROTATE_90)           # 270 horario
    if midp == 7: return img.transpose(T.FLIP_LEFT_RIGHT).transpose(T.ROTATE_270)
    if midp == 4: return img.transpose(T.FLIP_LEFT_RIGHT).transpose(T.ROTATE_90)
    return img


class Renderer:
    def __init__(s, images, palette_overrides=None):
        s.images = images; s.cache = {}; s.pal = palette_overrides or {}

    def sheet(s, img_id):
        key = (img_id, s.pal.get(img_id))
        if key not in s.cache:
            im = s.images[img_id]; png = im['png']
            sel = s.pal.get(img_id)
            if sel is not None and im['palettes']: png = png_with_palette(png, im['palettes'][sel])
            s.cache[key] = Image.open(io.BytesIO(png)).convert('RGBA')
        return s.cache[key]

    def piece(s, sprite, module_idx):
        m = sprite['modules'][module_idx]
        img_id, clip = (m >> 16) & 0xFFFF, m & 0xFFFF
        sh = s.sheet(img_id)
        clips = s.images[img_id]['clips']
        x, y, w, h = clips[clip] if clips else (0, 0, sh.width, sh.height)
        return sh.crop((x, y, x + w, y + h))

    def frame(s, sprite, fidx):
        """Devuelve (imagen RGBA, origen_x, origen_y): origen = punto (0,0) del frame."""
        f = sprite['frames'][fidx]
        parts = []
        for k in range(f['count']):
            v = sprite['frame_pieces'][f['start'] + k]
            mod = (v >> 18) & 1023; x = (v >> 9) & 511; y = v & 511; tr = (v >> 28) & 7
            if x & 256: x -= 512
            if y & 256: y -= 512
            img = transform(s.piece(sprite, mod), MIDP[tr])
            parts.append((img, x, y))
        if not parts: return None, 0, 0
        x0 = min(p[1] for p in parts); y0 = min(p[2] for p in parts)
        x1 = max(p[1] + p[0].width for p in parts); y1 = max(p[2] + p[0].height for p in parts)
        canvas = Image.new('RGBA', (x1 - x0, y1 - y0), (0, 0, 0, 0))
        for img, x, y in parts: canvas.alpha_composite(img, (x - x0, y - y0))
        return canvas, -x0, -y0


if __name__ == '__main__':
    sprites, images = load(sys.argv[1] if len(sys.argv) > 1 else 'apk_reference/king_fighter_iii/bin/animation.bin')
    for k, sp in enumerate(sprites):
        print(k, 'imgs', sp['images'][:6], 'mods', len(sp['modules']), 'frames', len(sp['frames']), 'acciones', len(sp['actions']))
