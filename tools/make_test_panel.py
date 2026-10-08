#!/usr/bin/env python3
"""Erzeugt das fest eingebaute Test-Panel fuer den Prototyp (Schritt 1).

Zeichnet ein comic-artiges Bild, skaliert es auf 200 px Breite, dithert es per
Floyd-Steinberg auf die Pebble-64-Farben-Palette (2 Bit pro Kanal) und
schreibt es als palettiertes PNG sowie als Base64-Modul fuer PebbleKit JS.

Aufruf:  python3 tools/make_test_panel.py [eingabe.png]
Ohne Eingabe wird ein synthetisches Testbild gezeichnet.
"""
import base64
import io
import os
import sys

from PIL import Image, ImageDraw, ImageFont

WIDTH = 200
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LEVELS = (0, 85, 170, 255)


def draw_synthetic():
    w, h = 400, 720  # wird auf 200x360 verkleinert -> hoeher als 228, testet Scrollen
    img = Image.new("RGB", (w, h))
    d = ImageDraw.Draw(img)
    for y in range(h):  # Himmel-Verlauf, zeigt das Dithering
        t = y / h
        d.line([(0, y), (w, y)], fill=(int(40 + 180 * t), int(120 + 100 * t), int(230 - 60 * t)))
    d.ellipse([280, 40, 360, 120], fill=(255, 220, 60))  # Sonne
    d.rectangle([0, 560, w, h], fill=(70, 150, 60))  # Wiese
    # Figur
    d.ellipse([130, 300, 270, 440], fill=(250, 200, 160), outline="black", width=6)
    d.ellipse([165, 345, 190, 370], fill="black")
    d.ellipse([210, 345, 235, 370], fill="black")
    d.arc([165, 360, 235, 415], 20, 160, fill="black", width=6)
    d.rectangle([150, 440, 250, 600], fill=(200, 40, 40), outline="black", width=6)
    # Sprechblase
    d.rounded_rectangle([30, 140, 370, 270], radius=40, fill="white", outline="black", width=6)
    d.polygon([(200, 268), (240, 268), (215, 305)], fill="white", outline="black")
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 38)
    except OSError:
        font = ImageFont.load_default()
    d.text((200, 205), "HALLO PEBBLE!", fill="black", font=font, anchor="mm")
    d.rectangle([0, 0, w - 1, h - 1], outline="black", width=10)  # Panelrahmen
    return img


def scale_to_width(img):
    h = round(img.height * WIDTH / img.width)
    return img.convert("RGB").resize((WIDTH, h), Image.LANCZOS)


def dither_pebble64(img):
    """Floyd-Steinberg auf 4 Stufen je Kanal -> Indexbild mit 64er-Palette."""
    w, h = img.size
    raw = img.tobytes()
    px = [[float(raw[i]), float(raw[i + 1]), float(raw[i + 2])] for i in range(0, len(raw), 3)]
    out = bytearray(w * h)
    for y in range(h):
        for x in range(w):
            i = y * w + x
            old = px[i]
            q = [min(3, max(0, int(round(c / 85.0)))) for c in old]
            out[i] = (q[0] << 4) | (q[1] << 2) | q[2]
            err = [old[k] - LEVELS[q[k]] for k in range(3)]
            for dx, dy, f in ((1, 0, 7), (-1, 1, 3), (0, 1, 5), (1, 1, 1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and ny < h:
                    n = px[ny * w + nx]
                    for k in range(3):
                        n[k] += err[k] * f / 16.0
    pal_img = Image.new("P", (w, h))
    palette = []
    for idx in range(64):
        palette += [LEVELS[(idx >> 4) & 3], LEVELS[(idx >> 2) & 3], LEVELS[idx & 3]]
    pal_img.putpalette(palette)
    pal_img.putdata(out)
    return pal_img


def main():
    src = Image.open(sys.argv[1]) if len(sys.argv) > 1 else draw_synthetic()
    pal = dither_pebble64(scale_to_width(src))
    buf = io.BytesIO()
    pal.save(buf, format="PNG", optimize=True)
    data = buf.getvalue()

    os.makedirs(os.path.join(ROOT, "tools", "out"), exist_ok=True)
    with open(os.path.join(ROOT, "tools", "out", "test_panel.png"), "wb") as f:
        f.write(data)
    with open(os.path.join(ROOT, "src", "pkjs", "test_panel.js"), "w") as f:
        f.write("// Automatisch erzeugt von tools/make_test_panel.py - nicht von Hand aendern.\n")
        f.write("// %dx%d px, %d Bytes PNG\n" % (pal.width, pal.height, len(data)))
        f.write("module.exports = '%s';\n" % base64.b64encode(data).decode("ascii"))
    print("Panel %dx%d, %d Bytes" % (pal.width, pal.height, len(data)))


if __name__ == "__main__":
    main()
