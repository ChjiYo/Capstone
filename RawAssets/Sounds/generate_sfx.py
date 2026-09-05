# -*- coding: utf-8 -*-
"""
Capstone SFX Generator
----------------------
표준 라이브러리만 사용하는 절차적 효과음 생성기.
44.1kHz / 16bit / 모노 WAV 파일을 이 스크립트와 같은 폴더에 출력한다.

실행:
    "C:/Epic_Game/UE_5.7/Engine/Binaries/ThirdParty/Python3/Win64/python.exe" generate_sfx.py

파라미터(길이/피치/게인)를 고쳐서 다시 실행하면 그대로 재생성된다.
"""

import math
import os
import random
import wave
import struct

SR = 44100
OUT_DIR = os.path.dirname(os.path.abspath(__file__))


# ---------------------------------------------------------------- 기본 유틸

def n(dur):
    return max(1, int(SR * dur))


def silence(dur):
    return [0.0] * n(dur)


def noise(dur, seed=0):
    rnd = random.Random(seed)
    return [rnd.uniform(-1.0, 1.0) for _ in range(n(dur))]


def curve(v, t):
    """v가 함수면 v(t), 아니면 상수."""
    return v(t) if callable(v) else v


def mix(*layers):
    """layers: (buffer, gain) 튜플들. 가장 긴 길이에 맞춰 합산."""
    length = max(len(b) for b, _ in layers)
    out = [0.0] * length
    for buf, gain in layers:
        for i, s in enumerate(buf):
            out[i] += s * gain
    return out


def place(dst, src, at, gain=1.0):
    """dst의 at초 위치에 src를 더한다(필요하면 길이 확장)."""
    start = n(at) if at > 0 else 0
    need = start + len(src)
    if need > len(dst):
        dst.extend([0.0] * (need - len(dst)))
    for i, s in enumerate(src):
        dst[start + i] += s * gain
    return dst


def concat(*bufs):
    out = []
    for b in bufs:
        out.extend(b)
    return out


# ---------------------------------------------------------------- 필터

def lowpass(x, cutoff):
    """1-pole 로우패스. cutoff는 상수 또는 f(t01)."""
    out = []
    prev = 0.0
    N = len(x)
    for i, s in enumerate(x):
        fc = max(20.0, min(curve(cutoff, i / N), SR * 0.45))
        a = 1.0 - math.exp(-2.0 * math.pi * fc / SR)
        prev += a * (s - prev)
        out.append(prev)
    return out


def highpass(x, cutoff):
    lp = lowpass(x, cutoff)
    return [a - b for a, b in zip(x, lp)]


def bandpass(x, freq, q=2.0):
    """State-variable 밴드패스. freq는 상수 또는 f(t01)."""
    low = 0.0
    band = 0.0
    out = []
    N = len(x)
    damp = 1.0 / max(0.5, q)
    for i, s in enumerate(x):
        f = max(30.0, min(curve(freq, i / N), 7000.0))
        F = min(1.0, 2.0 * math.sin(math.pi * f / SR))
        # 안정성을 위해 샘플당 2회 적분
        for _ in range(2):
            high = s - low - damp * band
            band += (F * 0.5) * high
            low += (F * 0.5) * band
        out.append(band)
    return out


def drive(x, amount=3.0):
    """tanh 새추레이션으로 거친 질감 추가."""
    k = math.tanh(amount)
    return [math.tanh(amount * s) / k for s in x]


# ---------------------------------------------------------------- 엔벨로프

def env_ad(x, attack=0.004, power=2.5, hold=0.0):
    """어택 후 지수 감쇠."""
    N = len(x)
    a = max(1, int(SR * attack))
    h = int(SR * hold)
    out = []
    for i, s in enumerate(x):
        if i < a:
            g = i / a
        elif i < a + h:
            g = 1.0
        else:
            t = (i - a - h) / max(1, (N - a - h))
            g = max(0.0, 1.0 - t) ** power
        out.append(s * g)
    return out


def env_swell(x, peak=0.45, power=2.0):
    """중간(peak 지점)에서 최대가 되는 휘두르기용 엔벨로프."""
    N = len(x)
    out = []
    for i, s in enumerate(x):
        t = i / N
        if t < peak:
            g = (t / peak) ** 1.6
        else:
            g = max(0.0, 1.0 - (t - peak) / (1.0 - peak)) ** power
        out.append(s * g)
    return out


def tremolo(x, rate, depth=0.5, seed=0):
    out = []
    for i, s in enumerate(x):
        m = 1.0 - depth * 0.5 * (1.0 - math.cos(2.0 * math.pi * rate * i / SR))
        out.append(s * m)
    return out


# ---------------------------------------------------------------- 오실레이터

def sine_sweep(dur, f0, f1, phase=0.0, exp=True):
    N = n(dur)
    ph = phase
    out = []
    for i in range(N):
        t = i / N
        f = f0 * (f1 / f0) ** t if exp else f0 + (f1 - f0) * t
        ph += 2.0 * math.pi * f / SR
        out.append(math.sin(ph))
    return out


def partials(dur, freqs_gains, decay_power=3.0, detune=0.0, seed=0):
    """비조화 배음(금속/타격음 링) 합성."""
    rnd = random.Random(seed)
    N = n(dur)
    out = [0.0] * N
    for f, g, dp in freqs_gains:
        f = f * (1.0 + rnd.uniform(-detune, detune))
        ph = rnd.uniform(0, 6.283)
        for i in range(N):
            t = i / N
            ph += 2.0 * math.pi * f / SR
            out[i] += g * math.sin(ph) * (max(0.0, 1.0 - t) ** dp)
    return out


def growl(dur, f0_a, f0_b, formants, rough=0.0, vib_rate=17.0, vib_depth=0.05, seed=0):
    """성대 펄스(naive saw) + 포먼트 밴드패스로 만든 몬스터 발성."""
    N = n(dur)
    rnd = random.Random(seed)
    ph = 0.0
    src = []
    for i in range(N):
        t = i / N
        f0 = f0_a * (f0_b / f0_a) ** t
        f0 *= 1.0 + vib_depth * math.sin(2.0 * math.pi * vib_rate * i / SR)
        if rough:
            f0 *= 1.0 + rough * rnd.uniform(-1.0, 1.0)
        ph = (ph + f0 / SR) % 1.0
        src.append(2.0 * ph - 1.0)
    out = [0.0] * N
    for fc, q, g in formants:
        b = bandpass(src, fc, q)
        for i in range(N):
            out[i] += g * b[i]
    return out


# ---------------------------------------------------------------- 출력

def dc_block(x, fc=20.0):
    """저역 DC 오프셋 제거(1-pole 하이패스). 30Hz 이하 서브베이스는 유지."""
    a = 1.0 - math.exp(-2.0 * math.pi * fc / SR)
    prev = 0.0
    out = []
    for s in x:
        prev += a * (s - prev)
        out.append(s - prev)
    return out


def rms_of(x):
    return math.sqrt(sum(s * s for s in x) / max(1, len(x)))


def normalize(x, peak=0.89, rms_ceiling_db=-14.0):
    """피크 정규화 후, 지나치게 큰 지속음은 RMS 상한으로 눌러 음량을 맞춘다."""
    m = max(abs(s) for s in x) or 1.0
    x = [s * (peak / m) for s in x]
    ceiling = 10.0 ** (rms_ceiling_db / 20.0)
    r = rms_of(x)
    if r > ceiling:
        k = ceiling / r
        x = [s * k for s in x]
    return x


def edge_fade(x, fade_in=0.0006, fade_out=0.005):
    """앞쪽 페이드는 타격 트랜지언트를 죽이지 않도록 아주 짧게."""
    N = len(x)
    fi = max(1, int(SR * fade_in))
    fo = max(1, int(SR * fade_out))
    for i in range(min(fi, N)):
        x[i] *= i / fi
    for i in range(min(fo, N)):
        x[N - 1 - i] *= i / fo
    return x


def save(name, buf):
    # 순서 중요: DC 제거 -> 페이드 -> 정규화 (정규화 후 페이드하면 피크가 깎인다)
    buf = normalize(edge_fade(dc_block(buf)))
    path = os.path.join(OUT_DIR, name + ".wav")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        frames = bytearray()
        for s in buf:
            v = int(max(-1.0, min(1.0, s)) * 32767)
            frames += struct.pack("<h", v)
        w.writeframes(bytes(frames))
    print("  %-34s %5.2fs  rms %5.1f dB" % (name + ".wav", len(buf) / SR,
                                            20.0 * math.log10(rms_of(buf) + 1e-9)))


# ================================================================ 빌딩 블록

def whoosh(dur, f_lo, f_hi, peak=0.44, q=1.7, seed=0, air=0.35, tail=0.0):
    """검/발톱/둔기를 휘두를 때의 바람 소리."""
    src = noise(dur, seed)

    # 진폭 정점보다 살짝 뒤에서 주파수 정점 -> 스쳐 지나가는 느낌
    def fc(t):
        s = math.sin(math.pi * min(1.0, t / max(0.05, (peak + 0.12)) * 0.9)) ** 1.25
        return f_lo + (f_hi - f_lo) * s

    body = env_swell(bandpass(src, fc, q), peak=peak, power=1.9)
    hi = env_swell(highpass(noise(dur, seed + 91), 3200), peak=peak + 0.05, power=2.6)
    out = mix((body, 1.0), (hi, air))
    if tail > 0.0:
        ring = env_ad(partials(tail, [(2460, 0.5, 4.0), (3710, 0.32, 4.6),
                                      (5230, 0.18, 5.2)], detune=0.02, seed=seed),
                      attack=0.001, power=4.0)
        out = place(out, ring, peak * dur, gain=0.22)
    return out


def transient(dur=0.012, cut=2600, seed=0, power=6.0):
    return env_ad(highpass(noise(dur, seed), cut), attack=0.0004, power=power)


def thump(dur, f0, f1, power=2.4):
    return env_ad(sine_sweep(dur, f0, f1), attack=0.002, power=power)


def gravel(dur, count, f_lo, f_hi, seed=0, spread=1.0):
    """돌/껍질 파편이 튀는 짧은 클릭 무리."""
    rnd = random.Random(seed)
    out = silence(dur)
    for i in range(count):
        at = rnd.uniform(0.0, dur * spread)
        d = rnd.uniform(0.004, 0.014)
        c = env_ad(bandpass(noise(d, seed + i * 7), rnd.uniform(f_lo, f_hi), q=5.0),
                   attack=0.0005, power=5.0)
        place(out, c, at, gain=rnd.uniform(0.25, 1.0) * (1.0 - at / dur) ** 1.5)
    return out


# ================================================================ 피격(Hit) 재료

def flesh_body(dur, cut_a, cut_b, seed, power=4.0, attack=0.0006):
    """살에 박히는 몸통.

    음정이 있는 사인 스윕(=북소리의 정체) 대신 빠르게 죽는 노이즈를 쓴다.
    노이즈는 기본 주파수가 없어서 아무리 저역이 들어가도 드럼으로 들리지 않는다.
    """
    return env_ad(lowpass(noise(dur, seed), lambda t: cut_a + (cut_b - cut_a) * t),
                  attack=attack, power=power)


def slice_cut(dur, f_hi, f_lo, seed, q=2.2):
    """날이 살을 가르고 지나가는 결. 위에서 아래로 떨어지는 좁은 노이즈."""
    return env_ad(bandpass(noise(dur, seed), lambda t: f_hi + (f_lo - f_hi) * t ** 0.6, q),
                  attack=0.0008, power=3.0)


def spray(dur, count, f_lo, f_hi, seed, delay=0.02):
    """체액이 튀는 잔물방울. 파편(gravel)보다 대역이 좁고 아래로 미끄러진다."""
    rnd = random.Random(seed)
    out = silence(dur)
    for i in range(count):
        at = delay + rnd.uniform(0.0, max(0.02, dur - delay - 0.02))
        d = rnd.uniform(0.006, 0.020)
        f = rnd.uniform(f_lo, f_hi)
        drop = env_ad(bandpass(noise(d, seed + i * 13),
                               lambda t, f=f: f * (1.0 - 0.45 * t), q=4.5),
                      attack=0.001, power=3.5)
        place(out, drop, at, gain=rnd.uniform(0.2, 0.7) * (1.0 - at / dur))
    return out


def bone_snap(dur, f, seed, q=7.0):
    """뼈·껍질이 부러지는 짧고 단단한 울림. 아주 빨리 죽어서 여운이 남지 않는다."""
    return env_ad(bandpass(noise(dur, seed), lambda t: f * (1.0 - 0.25 * t), q),
                  attack=0.0004, power=6.0)


# ================================================================ 플레이어 (검)

def steel_ring(dur, base, gain=1.0, seed=0, shimmer=0.004):
    """칼날이 울리는 강철 배음. 미세하게 디튠된 쌍으로 겹쳐 '샹' 하는 여운을 만든다."""
    ratios = [(1.00, 0.55, 2.2), (1.58, 0.40, 2.6), (2.27, 0.26, 3.0),
              (3.06, 0.15, 3.6), (4.11, 0.08, 4.2)]
    fg = []
    for r, g, dp in ratios:
        fg.append((base * r, g * gain, dp))
        fg.append((base * r * (1.0 + shimmer), g * gain * 0.65, dp))
    return highpass(partials(dur, fg, detune=0.006, seed=seed), 900)


def blade_swing(dur, f_lo, f_hi, peak=0.38, q=3.5, seed=0, rip=0.42,
                ring_base=2000, ring_gain=0.30, ring_len=0.32, low_cut=750):
    """중세 롱소드 스윙.

    좁은 밴드패스 2개로 칼날의 '결'을 살리고(넓은 노이즈 = 바람소리, 좁을수록 깔끔),
    low_cut 아래 저역과 11kHz 위 지글거림을 잘라내 날카롭고 정갈하게 만든다.
    """
    src = noise(dur, seed)

    def fc(t):
        # 진폭 정점 직후에 주파수가 최고점 -> 날이 귀 앞을 스쳐 지나가는 느낌
        u = min(1.0, t / max(0.08, peak + 0.10))
        return f_lo + (f_hi - f_lo) * math.sin(math.pi * u * 0.92) ** 1.15

    edge = bandpass(src, fc, q)
    edge2 = bandpass(src, lambda t: fc(t) * 1.62, q * 1.3)
    body = mix((edge, 1.0), (edge2, 0.45))
    body = highpass(lowpass(body, 11000), low_cut)
    body = env_swell(body, peak=peak, power=3.0)

    # 날 끝이 공기를 찢는 고운 하이
    air = env_swell(highpass(noise(dur, seed + 41), 6500), peak=peak + 0.04, power=4.2)

    out = mix((body, 1.0), (air, rip))
    ring = env_ad(steel_ring(ring_len, ring_base, seed=seed + 7), attack=0.0015, power=1.0)
    return place(out, ring, peak * dur * 1.05, gain=ring_gain)


def sword_swing(index):
    """4타 콤보. 1->2는 빠르고 높게, 3은 큰 호, 4는 마무리 회전베기."""
    presets = [
        # dur,  f_lo, f_hi, peak,  q,  rip, ring_base, ring_gain, ring_len, low_cut, seed
        (0.24, 1100, 3600, 0.36, 3.6, 0.42, 2100, 0.26, 0.28, 800, 11),
        (0.21, 1350, 4300, 0.33, 4.0, 0.50, 2480, 0.24, 0.24, 900, 24),
        (0.28,  900, 3100, 0.42, 3.2, 0.38, 1840, 0.32, 0.36, 700, 37),
    ]
    if index < 3:
        d, lo, hi, pk, q, rip, rb, rg, rl, lc, sd = presets[index]
        return blade_swing(d, lo, hi, peak=pk, q=q, seed=sd, rip=rip,
                           ring_base=rb, ring_gain=rg, ring_len=rl, low_cut=lc)
    return sword_swing_finisher()


def sword_swing_finisher():
    """4타: 회전 후 내려베기. 준비 바람 -> 본 스윙 -> 긴 강철 여운."""
    out = silence(0.42)
    # 짧은 준비 바람 (저역은 눌러 두어 탁해지지 않게)
    windup = env_swell(highpass(bandpass(noise(0.16, 50), lambda t: 700 + 900 * t, q=2.4), 450),
                       peak=0.62, power=2.4)
    place(out, windup, 0.0, 0.34)
    # 본 스윙
    place(out, blade_swing(0.34, 800, 4100, peak=0.34, q=3.0, seed=51, rip=0.55,
                           ring_base=1650, ring_gain=0.42, ring_len=0.62, low_cut=520),
          0.10, 1.0)
    return out


def sword_hit_flesh():
    """베어 들어가는 순간. 진흙 같은 저역 노이즈를 걷어내고 어택을 세웠다."""
    out = silence(0.34)
    place(out, transient(0.008, 4200, seed=5, power=7.0), 0.0, 0.85)
    place(out, env_ad(bandpass(noise(0.16, 6), lambda t: 2200 - 1300 * t, q=1.5),
                      attack=0.0008, power=3.4), 0.0, 0.95)
    # 저역은 "맞았다"는 무게만 남기고 최소한으로 (많으면 검이 아니라 둔기로 들린다)
    place(out, thump(0.14, 190, 62, power=3.0), 0.0, 0.30)
    place(out, env_ad(steel_ring(0.26, 2260, seed=12), attack=0.001, power=1.0), 0.002, 0.32)
    return out


def sword_hit_hard():
    """방패/바위/골렘에 튕기는 금속 타격."""
    out = silence(0.85)
    place(out, transient(0.010, 5000, seed=8, power=7.0), 0.0, 0.85)
    place(out, env_ad(steel_ring(0.75, 1960, seed=9, shimmer=0.006),
                      attack=0.0006, power=0.85), 0.0, 0.95)
    place(out, env_ad(bandpass(noise(0.10, 14), 3000, q=2.0), attack=0.0006, power=4.5),
          0.0, 0.40)
    place(out, thump(0.20, 150, 52, power=2.2), 0.0, 0.45)
    place(out, gravel(0.22, 7, 3000, 6000, seed=10), 0.008, 0.22)
    return out


# ================================================================ 몬스터

def goblin_attack():
    out = silence(0.52)
    v = growl(0.30, 540, 300, [(760, 6.0, 1.0), (1560, 7.0, 0.70), (2950, 8.0, 0.35)],
              rough=0.020, vib_rate=27.0, vib_depth=0.09, seed=21)
    place(out, env_ad(drive(v, 3.5), attack=0.010, power=2.2, hold=0.05), 0.0, 0.70)
    place(out, whoosh(0.24, 700, 3300, peak=0.40, q=2.0, seed=22, air=0.5, tail=0.16),
          0.16, 0.85)
    return out


def goblin_hit():
    """고블린: 마르고 작은 몸. 높고 짧고, 얕은 뼈에 날이 걸리는 딱 소리 + 단발 비명."""
    out = silence(0.34)
    place(out, transient(0.006, 5200, seed=101, power=8.0), 0.0, 0.55)
    place(out, slice_cut(0.14, 3400, 900, seed=102, q=2.6), 0.0, 0.85)
    place(out, flesh_body(0.10, 1400, 380, seed=103, power=5.0), 0.002, 0.75)
    place(out, bone_snap(0.05, 1750, seed=104, q=8.0), 0.006, 0.45)
    place(out, spray(0.20, 7, 1800, 4200, seed=105), 0.02, 0.30)
    place(out, env_ad(steel_ring(0.18, 2900, gain=0.8, seed=106), attack=0.001, power=1.2),
          0.0, 0.20)
    # 캑 하고 끊기는 짧은 비명
    yelp = growl(0.16, 900, 520, [(1250, 7.0, 1.0), (2400, 8.0, 0.55), (3600, 9.0, 0.25)],
                 rough=0.030, vib_rate=38.0, vib_depth=0.10, seed=107)
    place(out, env_ad(drive(yelp, 4.0), attack=0.006, power=3.0), 0.035, 0.55)
    return out


def larvae_attack():
    out = silence(0.55)
    g = growl(0.34, 135, 88, [(310, 4.0, 1.0), (860, 5.0, 0.45), (1700, 6.0, 0.18)],
              rough=0.075, vib_rate=11.0, vib_depth=0.10, seed=31)
    place(out, env_ad(lowpass(drive(g, 2.2), 1400), attack=0.030, power=1.8, hold=0.06),
          0.0, 0.75)
    # 젖은 스퀄치
    squelch = env_swell(
        bandpass(noise(0.26, 32),
                 lambda t: 380 + 1500 * math.sin(math.pi * t) ** 1.4, q=1.3),
        peak=0.40, power=2.2)
    place(out, lowpass(squelch, 2600), 0.14, 0.85)
    # 무는 순간
    place(out, transient(0.010, 1600, seed=33, power=5.0), 0.30, 0.45)
    place(out, thump(0.12, 150, 62, power=2.6), 0.30, 0.55)
    return out


def larvae_hit():
    """라바: 물주머니가 터지는 소리. 단단한 것이 하나도 없고 전부 젖어 있다."""
    out = silence(0.46)
    place(out, slice_cut(0.10, 2200, 700, seed=111, q=1.8), 0.0, 0.55)
    # 대역이 아래로 훅 꺼지는 젖은 파열
    place(out, env_ad(bandpass(noise(0.22, 112),
                               lambda t: 1500 * (1.0 - t) ** 1.6 + 180, q=1.2),
                      attack=0.0015, power=2.6), 0.0, 1.00)
    place(out, flesh_body(0.14, 900, 220, seed=113, power=3.4), 0.0, 0.70)
    place(out, spray(0.36, 16, 900, 3600, seed=114, delay=0.03), 0.02, 0.55)
    # 껍질이 우그러지며 삐걱임
    place(out, env_ad(bandpass(noise(0.18, 115),
                               lambda t: 620 + 280 * math.sin(math.pi * t * 3.0), q=6.0),
                      attack=0.004, power=2.4), 0.02, 0.30)
    place(out, env_ad(steel_ring(0.14, 2400, gain=0.6, seed=116), attack=0.001, power=1.4),
          0.0, 0.12)
    return out


def narbash_attack():
    out = silence(0.85)
    place(out, whoosh(0.40, 210, 1500, peak=0.55, q=1.4, seed=41, air=0.22), 0.0, 0.8)
    # 북/둔기 타격
    at = 0.30
    place(out, transient(0.014, 1800, seed=42, power=5.0), at, 0.55)
    place(out, env_ad(partials(0.50, [(92, 1.00, 2.6), (139, 0.62, 3.0),
                                      (208, 0.34, 3.6), (297, 0.18, 4.2)],
                               detune=0.01, seed=43), attack=0.001, power=1.0), at, 0.9)
    place(out, thump(0.45, 165, 60, power=1.8), at, 0.85)
    place(out, env_ad(lowpass(noise(0.22, 44), lambda t: 2400 * (1.0 - t) + 300),
                      attack=0.001, power=3.0), at, 0.35)
    return out


def narbash_hit():
    """나르바쉬: 덩치 큰 몸. 두꺼운 살과 가죽이라 무겁지만, 저역이 노이즈라 음정이 안 잡힌다."""
    out = silence(0.50)
    place(out, transient(0.010, 3200, seed=121, power=6.0), 0.0, 0.50)
    place(out, slice_cut(0.18, 2400, 600, seed=122, q=1.9), 0.0, 0.75)
    # 덩치의 무게는 서브베이스가 아니라 '두꺼운 중저역 + 빠른 감쇠'로 낸다.
    # 200Hz 아래를 길게 끌면 그 순간 북소리가 된다.
    place(out, flesh_body(0.16, 1100, 300, seed=123, power=3.6, attack=0.0012), 0.0, 1.00)
    # 가죽·모피가 눌리며 스치는 소리
    place(out, env_ad(bandpass(noise(0.16, 124), lambda t: 560 - 240 * t, q=1.1),
                      attack=0.004, power=2.6), 0.006, 0.38)
    place(out, spray(0.30, 10, 700, 2600, seed=125), 0.03, 0.30)
    # 배에서 밀려 나오는 낮은 신음. 기본 주파수는 잘라내고 포먼트만 남겨 목소리로만 들리게 한다
    grunt = growl(0.30, 190, 120, [(420, 4.5, 1.0), (980, 5.5, 0.50), (1900, 6.5, 0.20)],
                  rough=0.050, vib_rate=14.0, vib_depth=0.07, seed=126)
    place(out, env_ad(highpass(drive(grunt, 2.6), 220), attack=0.015, power=2.2), 0.03, 0.65)
    place(out, env_ad(steel_ring(0.22, 2050, gain=0.7, seed=127), attack=0.001, power=1.2),
          0.0, 0.18)
    return out


def bird_attack():
    out = silence(0.75)
    # 날갯짓 2회
    for i, at in enumerate((0.0, 0.135)):
        flap = env_swell(
            lowpass(noise(0.13, 51 + i),
                    lambda t: 900 + 1400 * math.sin(math.pi * t)),
            peak=0.45, power=2.0)
        place(out, flap, at, 0.55)
    # 날카로운 울음
    scr = growl(0.34, 1450, 880, [(2150, 9.0, 1.0), (3350, 10.0, 0.55), (4700, 10.0, 0.25)],
                rough=0.012, vib_rate=34.0, vib_depth=0.13, seed=52)
    scr = tremolo(drive(scr, 4.0), 42.0, depth=0.45)
    place(out, env_ad(scr, attack=0.012, power=2.0, hold=0.06), 0.10, 0.85)
    # 쪼기
    place(out, whoosh(0.16, 1200, 4200, peak=0.35, q=2.6, seed=53, air=0.6), 0.34, 0.6)
    place(out, transient(0.008, 2800, seed=54), 0.40, 0.35)
    return out


def bird_hit():
    """버드: 깃털이 터지고 속이 빈 가는 뼈가 부러진다. 가장 밝고 가볍다."""
    out = silence(0.42)
    place(out, transient(0.005, 6000, seed=131, power=8.0), 0.0, 0.60)
    place(out, slice_cut(0.12, 4200, 1500, seed=132, q=2.4), 0.0, 0.80)
    # 깃털 뭉치가 터져 흩어짐
    place(out, env_ad(highpass(noise(0.30, 133), lambda t: 2600 + 2200 * t),
                      attack=0.003, power=2.2), 0.004, 0.55)
    place(out, flesh_body(0.07, 1800, 500, seed=134, power=5.5), 0.0, 0.55)
    # 속 빈 뼈 두 대
    place(out, bone_snap(0.04, 2600, seed=135, q=9.0), 0.004, 0.50)
    place(out, bone_snap(0.03, 3400, seed=136, q=9.0), 0.012, 0.30)
    screech = growl(0.22, 2100, 1200, [(2900, 10.0, 1.0), (4300, 11.0, 0.50), (5900, 11.0, 0.22)],
                    rough=0.020, vib_rate=46.0, vib_depth=0.16, seed=137)
    place(out, env_ad(tremolo(drive(screech, 4.5), 55.0, depth=0.5),
                      attack=0.005, power=2.6), 0.02, 0.70)
    place(out, env_ad(steel_ring(0.16, 3200, gain=0.7, seed=138), attack=0.001, power=1.3),
          0.0, 0.20)
    return out


def bug_attack():
    out = silence(0.60)
    # 딱딱거리는 턱 소리
    rnd = random.Random(61)
    t = 0.0
    while t < 0.30:
        d = rnd.uniform(0.005, 0.010)
        c = env_ad(bandpass(noise(d, int(t * 9000) + 61), rnd.uniform(2400, 5200), q=6.0),
                   attack=0.0004, power=5.0)
        place(out, c, t, rnd.uniform(0.4, 1.0))
        t += rnd.uniform(0.022, 0.042)
    # 껍질 마찰
    place(out, env_swell(bandpass(noise(0.30, 62), lambda tt: 1500 + 900 * tt, q=2.2),
                         peak=0.5, power=2.0), 0.02, 0.28)
    # 찌르기
    place(out, whoosh(0.16, 950, 3900, peak=0.36, q=2.4, seed=63, air=0.5), 0.30, 0.8)
    place(out, transient(0.009, 2600, seed=64), 0.36, 0.45)
    place(out, thump(0.12, 190, 70, power=2.6), 0.36, 0.45)
    return out


def golem_slam():
    """골렘이 내려찍는 순간의 임팩트 (golem_attack 내부에서 사용)."""
    out = silence(1.10)
    place(out, transient(0.018, 2000, seed=81, power=4.0), 0.0, 0.7)
    place(out, env_ad(lowpass(noise(0.45, 82), lambda t: 2600 * (1.0 - t) ** 1.5 + 240),
                      attack=0.001, power=2.4), 0.0, 0.85)
    place(out, thump(0.85, 98, 30, power=1.5), 0.0, 1.0)
    place(out, env_ad(partials(0.40, [(155, 0.6, 2.6), (233, 0.34, 3.2), (388, 0.18, 3.8)],
                               detune=0.04, seed=83), attack=0.001, power=1.0), 0.0, 0.45)
    place(out, gravel(0.60, 22, 800, 4200, seed=84, spread=0.9), 0.05, 0.40)
    return out


def golem_hit():
    """골렘: 검이 돌에 튕긴다. 살점 요소는 하나도 없고 강철 링 + 균열 + 파편뿐."""
    out = silence(0.80)
    place(out, transient(0.009, 5200, seed=141, power=7.0), 0.0, 0.90)
    # 표면이 갈라지는 중역 균열
    place(out, env_ad(bandpass(noise(0.30, 142),
                               lambda t: 900 * (1.0 - t) ** 1.2 + 260, q=1.6),
                      attack=0.0008, power=2.6), 0.0, 0.95)
    # 튕긴 검날이 우는 강철
    place(out, env_ad(steel_ring(0.62, 2150, gain=1.0, seed=143, shimmer=0.007),
                      attack=0.0006, power=0.9), 0.0, 0.85)
    # 무게는 짧게만. 길게 끌면 그게 다시 북소리가 된다
    place(out, env_ad(lowpass(noise(0.13, 144), 300), attack=0.001, power=3.2), 0.0, 0.55)
    place(out, gravel(0.55, 20, 1200, 5200, seed=145, spread=0.85), 0.015, 0.50)
    return out


def golem_attack():
    out = silence(1.00)
    # 돌이 갈리며 팔을 들어올림
    grind = env_swell(lowpass(noise(0.55, 71), lambda t: 700 + 500 * t), peak=0.55, power=1.8)
    place(out, drive(grind, 2.0), 0.0, 0.60)
    place(out, gravel(0.50, 14, 900, 3200, seed=72), 0.02, 0.35)
    # 저역 울림
    rumble = tremolo(mix((sine_sweep(0.60, 44, 38), 1.0), (sine_sweep(0.60, 63, 55), 0.5)),
                     7.0, depth=0.5)
    place(out, env_swell(rumble, peak=0.5, power=1.6), 0.0, 0.55)
    # 내려찍기
    place(out, golem_slam(), 0.48, 0.95)
    return out


# ================================================================ 엔트리 포인트

SOUNDS = [
    # ---- 플레이어 (검) ----
    ("SFX_Player_SwordSwing_01", lambda: sword_swing(0)),
    ("SFX_Player_SwordSwing_02", lambda: sword_swing(1)),
    ("SFX_Player_SwordSwing_03", lambda: sword_swing(2)),
    ("SFX_Player_SwordSwing_04", lambda: sword_swing(3)),
    ("SFX_Player_SwordHit_Flesh", sword_hit_flesh),
    ("SFX_Player_SwordHit_Hard", sword_hit_hard),
    # ---- 몬스터 ----
    ("SFX_Mon_Goblin_Attack", goblin_attack),
    ("SFX_Mon_Goblin_Hit", goblin_hit),
    ("SFX_Mon_Larvae_Attack", larvae_attack),
    ("SFX_Mon_Larvae_Hit", larvae_hit),
    ("SFX_Mon_Narbash_Attack", narbash_attack),
    ("SFX_Mon_Narbash_Hit", narbash_hit),
    ("SFX_Mon_Bird_Attack", bird_attack),
    ("SFX_Mon_Bird_Hit", bird_hit),
    ("SFX_Mon_Bug_Attack", bug_attack),
    ("SFX_Mon_Golem_Attack", golem_attack),
    ("SFX_Mon_Golem_Hit", golem_hit),
]


def main():
    print("Capstone SFX -> %s" % OUT_DIR)
    for name, fn in SOUNDS:
        save(name, fn())
    print("done: %d files" % len(SOUNDS))


if __name__ == "__main__":
    main()
