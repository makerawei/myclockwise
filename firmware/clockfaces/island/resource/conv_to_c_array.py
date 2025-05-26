# -*- coding: utf-8 -*-
import sys
from PIL import Image


def rgb888_to_rgb565(r, g, b):
    """将8位RGB转为16位RGB565"""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def image_to_rgb565_array(image_path):
    img = Image.open(image_path)
    w, h = img.size
    img = img.resize((w, h), Image.NEAREST)
    img = img.convert('RGB')
    pixels = list(img.getdata())
    rgb565_array = [f"0x{rgb888_to_rgb565(r, g, h):04x}" for (r, g, h) in pixels]
    return w, h, rgb565_array


if __name__ == "__main__":
    w, h, arr = image_to_rgb565_array(sys.argv[1])
    print(f"{w}, {h}")
    code = ""
    for i in range(h):
        line = arr[i * w : (i + 1) * w]
        code += (', '.join(line) + ",\n")
        
    print(code[:-2])