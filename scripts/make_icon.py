"""Draws Resources/icon.png: a red matryoshka on a black lacquer tile with a gold rim."""
import math
from PIL import Image, ImageDraw, ImageFilter

S = 1024
SS = 2  # supersample
W = S * SS
img = Image.new("RGBA", (W, W), (0, 0, 0, 0))
d = ImageDraw.Draw(img)

def sc(v):
    return v * SS

# lacquer tile
d.rounded_rectangle([sc(40), sc(40), sc(984), sc(984)], radius=sc(200), fill=(21, 16, 14, 255), outline=(226, 173, 59, 255), width=sc(14))

def bez(p0, p1, p2, p3, n=40):
    pts = []
    for i in range(n + 1):
        t = i / n
        a = (1 - t) ** 3; b = 3 * t * (1 - t) ** 2; c = 3 * t * t * (1 - t); e = t ** 3
        pts.append((a * p0[0] + b * p1[0] + c * p2[0] + e * p3[0], a * p0[1] + b * p1[1] + c * p2[1] + e * p3[1]))
    return pts

cx, top, w, h = 512, 150, 520, 760
def P(x, y): return (sc(cx + x * w), sc(top + y * h))
right = []
right += bez(P(0, 0), P(.31, 0), P(.37, .2), P(.33, .36))
right += bez(P(.33, .36), P(.31, .43), P(.5, .55), P(.49, .8))
right += bez(P(.49, .8), P(.48, .94), P(.4, 1), P(.3, 1))
left = [(sc(cx) * 2 - x, y) for (x, y) in right]
outline = right + list(reversed(left))

mask = Image.new("L", (W, W), 0)
ImageDraw.Draw(mask).polygon(outline, fill=255)
body = Image.new("RGBA", (W, W), (200, 31, 45, 255))
bd = ImageDraw.Draw(body)
# scarf
bd.ellipse([sc(cx - .4 * w), sc(top - .04 * h), sc(cx + .4 * w), sc(top + .46 * h)], fill=(240, 179, 36, 255))
for i in range(9):
    a = math.pi * (.15 + .7 * i / 8)
    x = cx - math.cos(a) * .31 * w; y = top + .2 * h - math.sin(a) * .17 * h
    bd.ellipse([sc(x - 10), sc(y - 10), sc(x + 10), sc(y + 10)], fill=(200, 140, 20, 255))
# apron + flower
bd.ellipse([sc(cx - .31 * w), sc(top + .5 * h), sc(cx + .31 * w), sc(top + 1.06 * h)], fill=(244, 228, 193, 255))
fx, fy, pr = cx, top + .72 * h, .075 * w
for side in (-1, 1):
    bd.ellipse([sc(fx + side * pr * 2 - pr * .7), sc(fy + pr * .2), sc(fx + side * pr * 2 + pr * .7), sc(fy + pr * 3)], fill=(47, 143, 78, 255))
for i in range(5):
    a = 2 * math.pi * i / 5
    x = fx + math.sin(a) * pr * 1.25; y = fy - math.cos(a) * pr * 1.25
    bd.ellipse([sc(x - pr), sc(y - pr), sc(x + pr), sc(y + pr)], fill=(200, 31, 45, 255))
bd.ellipse([sc(fx - pr * .9), sc(fy - pr * .9), sc(fx + pr * .9), sc(fy + pr * .9)], fill=(226, 173, 59, 255))
for i in range(11):
    t = i / 10
    x = cx - .3 * w + t * .6 * w; y = top + .44 * h + math.sin(t * math.pi) * .02 * h
    bd.ellipse([sc(x - 7), sc(y - 7), sc(x + 7), sc(y + 7)], fill=(226, 173, 59, 255))
img.paste(body, (0, 0), mask)

# face
fx0, fy0, fw, fh = cx - .2 * w, top + .09 * h, .4 * w, .25 * h
d.ellipse([sc(fx0), sc(fy0), sc(fx0 + fw), sc(fy0 + fh)], fill=(248, 220, 192, 255), outline=(36, 21, 16, 255), width=sc(3))
hair = Image.new("L", (W, W), 0)
ImageDraw.Draw(hair).ellipse([sc(fx0), sc(fy0), sc(fx0 + fw), sc(fy0 + fh)], fill=255)
hl = Image.new("RGBA", (W, W), (0, 0, 0, 0))
ImageDraw.Draw(hl).ellipse([sc(cx - .22 * w), sc(top + .05 * h), sc(cx + .22 * w), sc(top + .17 * h)], fill=(90, 50, 28, 255))
img.paste(hl, (0, 0), Image.composite(hl.split()[3], Image.new("L", (W, W), 0), hair))
ey = top + .205 * h
for side in (-1, 1):
    x = cx + side * .07 * w
    d.ellipse([sc(x - 11), sc(ey - 14), sc(x + 11), sc(ey + 14)], fill=(36, 21, 16, 255))
    d.ellipse([sc(x - 2), sc(ey - 10), sc(x + 6), sc(ey - 2)], fill=(255, 255, 255, 255))
    cxk = cx + side * .115 * w; cy = top + .255 * h
    d.ellipse([sc(cxk - 22), sc(cy - 22), sc(cxk + 22), sc(cy + 22)], fill=(232, 118, 138, 220))
d.ellipse([sc(cx - 14), sc(top + .28 * h), sc(cx + 14), sc(top + .3 * h)], fill=(160, 20, 30, 255))
d.line(outline + [outline[0]], fill=(36, 21, 16, 255), width=sc(5))

img = img.resize((S, S), Image.LANCZOS)
img.save("Resources/icon.png")
print("wrote Resources/icon.png")
