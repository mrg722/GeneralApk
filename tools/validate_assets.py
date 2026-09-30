from pathlib import Path
import json
import struct
import zlib

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {
    "assets/characters/rayden_clean.png": (384, 384, (8, 6)),
    "assets/backgrounds/old_steel_yard_clean.png": (1280, 720, (8, 2),),
}
OPTIONAL_SCENARIOS = {
    "assets/backgrounds/mercado_antiguo_clean.png": (256, 144),
    "assets/backgrounds/zona_quimica_clean.png": (256, 144),
}
ENEMY_ATLAS_NAMES = ("punk", "charger", "brute", "enforcer", "chemical_soldier", "urban_ninja", "mutant", "armored_guard")
EXPECTED_ENEMY_CLIPS = {"idle": [0,1,2,3], "walk": [0,1,2,3], "attack": [4,5,6,7], "hit": [8,9], "defeat": [10,11]}
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"

def png_info(path: Path):
    raw = path.read_bytes()
    assert raw.startswith(PNG_SIGNATURE), f"{path}: no es un PNG"
    length = struct.unpack(">I", raw[8:12])[0]
    assert raw[12:16] == b"IHDR" and length >= 13, f"{path}: falta IHDR"
    return struct.unpack(">IIBB", raw[16:26])

def read_rgba(path: Path):
    raw = path.read_bytes(); pos=8; width=height=depth=color_type=None; idat=bytearray()
    while pos < len(raw):
        length=struct.unpack(">I",raw[pos:pos+4])[0]; kind=raw[pos+4:pos+8]; data=raw[pos+8:pos+8+length]; pos += 12+length
        if kind==b"IHDR": width,height,depth,color_type,_,_,_=struct.unpack(">IIBBBBB",data)
        elif kind==b"IDAT": idat.extend(data)
        elif kind==b"IEND": break
    assert depth==8 and color_type==6, f"{path}: para medir alpha se requiere RGBA PNG 8-bit"
    decoded=zlib.decompress(bytes(idat)); stride=width*4; rows=[]; prev=bytearray(stride); offset=0
    for _ in range(height):
        filter_type=decoded[offset]; offset+=1; row=bytearray(decoded[offset:offset+stride]); offset+=stride
        for i in range(stride):
            left=row[i-4] if i>=4 else 0; up=prev[i]; up_left=prev[i-4] if i>=4 else 0
            if filter_type==1: row[i]=(row[i]+left)&255
            elif filter_type==2: row[i]=(row[i]+up)&255
            elif filter_type==3: row[i]=(row[i]+((left+up)//2))&255
            elif filter_type==4:
                p=left+up-up_left; pa=abs(p-left); pb=abs(p-up); pc=abs(p-up_left); pr=left if pa<=pb and pa<=pc else up if pb<=pc else up_left; row[i]=(row[i]+pr)&255
            elif filter_type!=0: raise AssertionError(f"{path}: filtro PNG no soportado: {filter_type}")
        rows.append(row); prev=row
    return width,height,rows

def validate_enemy_atlas(path: Path):
    width,height,rows=read_rgba(path); assert (width,height)==(512,384), f"{path}: se esperaba 512x384, llegó {width}x{height}"
    for frame in range(12):
        x0=(frame%4)*128; y0=(frame//4)*128; min_x=min_y=128; max_x=max_y=-1
        for y in range(128):
            row=rows[y0+y]
            for x in range(128):
                if row[(x0+x)*4+3]>=8: min_x=min(min_x,x); min_y=min(min_y,y); max_x=max(max_x,x); max_y=max(max_y,y)
        assert max_x>=min_x and max_y>=min_y, f"{path}: frame {frame} está vacío"

def main():
    manifest=json.loads((ROOT/"data"/"sprite_manifest.json").read_text(encoding="utf-8")); atlases=manifest["atlases"]; clips=manifest["clips"]["enemy_default"]
    for name in ENEMY_ATLAS_NAMES:
        atlas=atlases[name]; assert atlas["grid"]==[4,3]; assert atlas["cell"]==[128,128]; assert atlas["path"]==f"assets/enemies/{name}_clean.png"
        p=ROOT/atlas["path"]; assert p.is_file(), f"{p}: falta el atlas enemigo obligatorio"; width,height,depth,color_type=png_info(p); assert (width,height)==(512,384); assert depth==8 and color_type==6; validate_enemy_atlas(p)
    for clip_name,frames in EXPECTED_ENEMY_CLIPS.items(): assert clips[clip_name]["frames"]==frames
    for rel,(w,h,types) in EXPECTED.items():
        p=ROOT/rel; assert p.is_file(), f"{rel}: falta el asset obligatorio"; width,height,depth,color_type=png_info(p); assert (width,height)==(w,h); assert depth in types; assert (color_type==6 if rel.endswith("rayden_clean.png") else color_type==2)
    for rel,(w,h) in OPTIONAL_SCENARIOS.items():
        p=ROOT/rel
        if p.is_file():
            width,height,depth,color_type=png_info(p); assert (width,height)==(w,h), f"{rel}: llegó {width}x{height}, se esperaba {w}x{h}"; assert depth in (4,8), f"{rel}: profundidad PNG no soportada para fondo pixel: {depth}"; assert color_type in (3,6), f"{rel}: tipo de color PNG no soportado para fondo pixel: {color_type}"
    for name,atlas in atlases.items(): assert "path" in atlas and atlas["path"].endswith(".png")
    print("OK: atlas enemigos, manifest y assets visuales presentes fueron validados")

if __name__=="__main__": main()
