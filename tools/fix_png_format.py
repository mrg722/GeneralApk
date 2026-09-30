#!/usr/bin/env python3
"""Script to convert PNG assets to RGBA format."""

from PIL import Image
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# List of PNG files that need to be converted to RGBA
ASSETS_TO_FIX = [
    "assets/backgrounds/old_steel_yard_clean.png",
]

def convert_to_rgba(image_path):
    """Convert PNG image to RGBA format."""
    try:
        img = Image.open(image_path)
        print(f"Processing: {image_path}")
        print(f"  Original mode: {img.mode}, Size: {img.size}")
        
        # Convert to RGBA
        img_rgba = img.convert('RGBA')
        
        # Save the converted image
        img_rgba.save(image_path)
        print(f"  Converted to RGBA and saved successfully")
        return True
    except Exception as e:
        print(f"  Error: {e}")
        return False

if __name__ == "__main__":
    print("Converting PNG assets to RGBA format...\n")
    
    for asset in ASSETS_TO_FIX:
        image_path = ROOT / asset
        if image_path.exists():
            convert_to_rgba(image_path)
        else:
            print(f"File not found: {asset}")
    
    print("\nDone!")
