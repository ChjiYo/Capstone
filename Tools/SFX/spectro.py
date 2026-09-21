"""SoundSource/*.wav 의 스펙트로그램 컨택트 시트를 PNG 로 만든다 (귀 대신 눈으로 검수). python spectro.py out_prefix [filter...]"""
import os, sys, glob, wave
import numpy as np
from PIL import Image, ImageDraw
from scipy import signal

SRC = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "SoundSource"))

def load(p):
    w = wave.open(p); n = w.getnframes(); ch = w.getnchannels(); sr = w.getframerate()
    x = np.frombuffer(w.readframes(n), dtype="<i2").astype(float) / 32768
    return x.reshape(-1, ch).mean(axis=1), sr

def tile(p, W=420, H=150):
    x, sr = load(p)
    f, t, S = signal.spectrogram(x, sr, nperseg=1024, noverlap=768)
    S = 10 * np.log10(S + 1e-12)
    S = np.clip((S - (S.max() - 80)) / 80, 0, 1)
    fmax = min(sr / 2, 12000)
    S = S[f <= fmax][::-1]
    img = Image.fromarray((S * 255).astype(np.uint8)).resize((W, H - 14), Image.BILINEAR).convert("RGB")
    # 간단한 컬러맵 (검정->보라->주황->노랑)
    a = np.asarray(img)[..., 0] / 255.0
    rgb = np.stack([np.clip(a * 2.0, 0, 1), np.clip(a * 1.2 - 0.2, 0, 1) ** 1.5, np.clip(1.0 - np.abs(a - 0.3) * 3, 0, 1) * 0.8], -1)
    canvas = Image.new("RGB", (W, H), (20, 20, 20))
    canvas.paste(Image.fromarray((rgb * 255).astype(np.uint8)), (0, 14))
    d = ImageDraw.Draw(canvas)
    d.text((3, 1), f"{os.path.basename(p)[:-4]}  {len(x)/sr:.2f}s", fill=(255, 255, 255))
    return canvas

def main():
    prefix = sys.argv[1]
    flt = sys.argv[2:]
    files = sorted(glob.glob(os.path.join(SRC, "*.wav")))
    if flt:
        files = [f for f in files if any(k in os.path.basename(f) for k in flt)]
    cols = 3
    tiles = [tile(f) for f in files]
    rows = (len(tiles) + cols - 1) // cols
    W, H = tiles[0].size
    sheet = Image.new("RGB", (W * cols, H * rows), (0, 0, 0))
    for i, t in enumerate(tiles):
        sheet.paste(t, ((i % cols) * W, (i // cols) * H))
    out = os.path.join(os.environ.get("SFX_TMP", "."), prefix + ".png")
    sheet.save(out)
    print(out, len(tiles))

main()
