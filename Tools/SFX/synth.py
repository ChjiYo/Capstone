"""절차적 사운드 합성용 작은 라이브러리 (numpy + scipy 전용, 외부 샘플 없음).

make_sfx.py 의 레시피들이 이 모듈의 프리미티브(오실레이터, 엔벨로프, 필터, 리버브 ...)를 조합해
게임용 SFX WAV를 만든다. 모든 난수는 이름에서 뽑은 시드로 고정되므로 다시 돌려도 같은 결과가 나온다.
"""
import zlib
import wave
import numpy as np
from scipy import signal

SR = 44100


def ns(dur, sr=SR):
    return int(round(dur * sr))


def rng_for(name):
    return np.random.default_rng(zlib.crc32(name.encode("utf-8")))


def tt(dur, sr=SR):
    return np.arange(int(round(dur * sr))) / sr


# ---------------------------------------------------------------- 오실레이터
def phase_of(freq, sr=SR):
    """freq: 스칼라 또는 배열(Hz). 위상(rad) 배열을 돌려준다 (배열이면 순간 주파수를 적분)."""
    return 2 * np.pi * np.cumsum(freq) / sr


def sine(freq, dur, sr=SR, ph0=0.0):
    n = int(round(dur * sr))
    f = np.full(n, float(freq)) if np.isscalar(freq) else np.asarray(freq, dtype=float)
    return np.sin(phase_of(f, sr) + ph0)


def chirp(f0, f1, dur, sr=SR, curve="exp"):
    """f0 -> f1 로 미끄러지는 사인. curve='exp'(음정 체감상 균일) 또는 'lin'."""
    n = int(round(dur * sr))
    x = np.linspace(0.0, 1.0, n)
    if curve == "exp":
        f = f0 * (f1 / f0) ** x
    else:
        f = f0 + (f1 - f0) * x
    return np.sin(phase_of(f, sr))


def saw_bl(freq, dur, sr=SR, max_hz=6000.0, ph0=0.0):
    """대역 제한 톱니파 (가산합성). freq 스칼라/배열."""
    n = int(round(dur * sr))
    f = np.full(n, float(freq)) if np.isscalar(freq) else np.asarray(freq, dtype=float)
    ph = phase_of(f, sr) + ph0
    fmin = max(float(np.min(f)), 20.0)
    kmax = int(min(max_hz / fmin, 60))
    out = np.zeros(n)
    for k in range(1, kmax + 1):
        gate = (f * k < max_hz).astype(float)
        out += gate * np.sin(k * ph) / k
    return out * (2 / np.pi)


def square_bl(freq, dur, sr=SR, max_hz=6000.0):
    n = int(round(dur * sr))
    f = np.full(n, float(freq)) if np.isscalar(freq) else np.asarray(freq, dtype=float)
    ph = phase_of(f, sr)
    fmin = max(float(np.min(f)), 20.0)
    kmax = int(min(max_hz / fmin, 60))
    out = np.zeros(n)
    for k in range(1, kmax + 1, 2):
        gate = (f * k < max_hz).astype(float)
        out += gate * np.sin(k * ph) / k
    return out * (4 / np.pi) * 0.5


def white(dur, rng, sr=SR):
    return rng.standard_normal(int(round(dur * sr)))


def pink(dur, rng, sr=SR):
    """Voss-McCartney 대신 FFT 1/f 셰이핑 (짧은 소리엔 충분, 루프 전체에도 사용)."""
    n = int(round(dur * sr))
    x = rng.standard_normal(n)
    spec = np.fft.rfft(x)
    f = np.fft.rfftfreq(n, 1 / sr)
    f[0] = f[1]
    spec /= np.sqrt(f)
    y = np.fft.irfft(spec, n)
    return y / (np.max(np.abs(y)) + 1e-9)


def brown(dur, rng, sr=SR):
    n = int(round(dur * sr))
    x = rng.standard_normal(n)
    spec = np.fft.rfft(x)
    f = np.fft.rfftfreq(n, 1 / sr)
    f[0] = f[1]
    spec /= f
    y = np.fft.irfft(spec, n)
    return y / (np.max(np.abs(y)) + 1e-9)


# ---------------------------------------------------------------- 엔벨로프
def env_exp(n, tau, attack=0.002, sr=SR):
    """지수 감쇠 + 짧은 선형 어택(클릭 방지)."""
    t = np.arange(n) / sr
    e = np.exp(-t / max(tau, 1e-4))
    a = max(int(attack * sr), 1)
    e[:a] *= np.linspace(0.0, 1.0, a)
    return e


def env_adsr(n, a, d, s, r, sr=SR):
    a_n, d_n, r_n = int(a * sr), int(d * sr), int(r * sr)
    s_n = max(n - a_n - d_n - r_n, 0)
    e = np.concatenate([
        np.linspace(0, 1, max(a_n, 1)),
        np.linspace(1, s, max(d_n, 1)),
        np.full(s_n, s),
        np.linspace(s, 0, max(r_n, 1)),
    ])
    if len(e) < n:
        e = np.pad(e, (0, n - len(e)))
    return e[:n]


def env_bell(n, peak_at=0.35, power=2.0):
    """0 -> 1 -> 0 의 부드러운 종 모양 (스윕/스웰용)."""
    x = np.linspace(0, 1, n)
    up = (x / peak_at) ** power
    dn = ((1 - x) / (1 - peak_at)) ** power
    return np.where(x < peak_at, up, dn)


def smooth_noise(n, rate_hz, rng, sr=SR):
    """rate_hz 근처로 천천히 출렁이는 0~1 난수 곡선 (게인/컷오프 변조용)."""
    knots = max(int(n / sr * rate_hz) + 3, 4)
    pts = rng.random(knots)
    xs = np.linspace(0, n - 1, knots)
    # 코사인 보간으로 부드럽게
    xi = np.arange(n)
    idx = np.clip(np.searchsorted(xs, xi, side="right") - 1, 0, knots - 2)
    frac = (xi - xs[idx]) / (xs[idx + 1] - xs[idx])
    frac = (1 - np.cos(np.pi * frac)) / 2
    return pts[idx] * (1 - frac) + pts[idx + 1] * frac


# ---------------------------------------------------------------- 필터
def _sos(kind, cutoff, sr, order):
    return signal.butter(order, cutoff, btype=kind, fs=sr, output="sos")


def lp(x, hz, order=2, sr=SR):
    return signal.sosfilt(_sos("low", min(hz, sr / 2 - 100), sr, order), x)


def hp(x, hz, order=2, sr=SR):
    return signal.sosfilt(_sos("high", hz, sr, order), x)


def bp(x, lo, hi, order=2, sr=SR):
    return signal.sosfilt(_sos("band", [lo, min(hi, sr / 2 - 100)], sr, order), x)


def resonator(x, hz, q=8.0, sr=SR):
    """2극 공진 밴드패스 (포먼트/금속성 울림용). 게인은 대략 정규화."""
    w0 = 2 * np.pi * hz / sr
    alpha = np.sin(w0) / (2 * q)
    b = np.array([alpha, 0.0, -alpha])
    a = np.array([1 + alpha, -2 * np.cos(w0), 1 - alpha])
    return signal.lfilter(b / a[0], a / a[0], x)


def swept_bp(x, f_start, f_end, q=3.0, sr=SR, block=256):
    """중심주파수가 시간에 따라 변하는 밴드패스 (휘슬/우슈 스윕용). 블록 단위 상태 유지 필터."""
    n = len(x)
    out = np.zeros(n)
    zi = np.zeros(2)
    for s in range(0, n, block):
        e = min(s + block, n)
        frac = (s + (e - s) / 2) / n
        f = f_start * (f_end / f_start) ** frac
        w0 = 2 * np.pi * f / sr
        alpha = np.sin(w0) / (2 * q)
        b = np.array([alpha, 0.0, -alpha])
        a = np.array([1 + alpha, -2 * np.cos(w0), 1 - alpha])
        y, zi = signal.lfilter(b / a[0], a / a[0], x[s:e], zi=zi)
        out[s:e] = y
    return out


def drive(x, amount=2.0):
    return np.tanh(x * amount) / np.tanh(amount)


# ---------------------------------------------------------------- 공간/믹스
def reverb(x, rt60=0.8, mix=0.25, predelay=0.012, damp_hz=5000.0, sr=SR, seed=1):
    """합성 임펄스 응답 컨볼루션 리버브. 모노 입력 -> 스테레오 출력(2, n)."""
    rng = np.random.default_rng(seed)
    n_ir = int(rt60 * 1.2 * sr)
    outs = []
    for ch in range(2):
        ir = rng.standard_normal(n_ir)
        t = np.arange(n_ir) / sr
        ir *= np.exp(-6.9 * t / rt60)
        ir = lp(ir, damp_hz, 1, sr)
        pd = int(predelay * sr * (1 + 0.3 * ch))
        ir = np.concatenate([np.zeros(pd), ir])
        ir /= np.sqrt(np.sum(ir ** 2)) + 1e-9
        wet = signal.fftconvolve(x, ir)
        outs.append(wet)
    n = max(len(o) for o in outs)
    wet = np.stack([np.pad(o, (0, n - len(o))) for o in outs])
    dry = np.pad(x, (0, n - len(x)))
    return dry[None, :] * (1 - mix) + wet * mix * 3.0


def echo(x, delay=0.18, feedback=0.35, taps=4, sr=SR):
    d = int(delay * sr)
    out = np.pad(x, (0, d * taps))
    for k in range(1, taps + 1):
        out[k * d:k * d + len(x)] += x * (feedback ** k)
    return out


def place(parts, total_dur, sr=SR):
    """[(offset_sec, signal, gain), ...] 를 하나의 버퍼에 합친다 (모노)."""
    n = int(round(total_dur * sr))
    out = np.zeros(n)
    for off, sig, gain in parts:
        s = int(round(off * sr))
        if s >= n:
            continue
        e = min(s + len(sig), n)
        seg = sig[:e - s].copy()
        # 각 조각의 끝이 0 이 아닌 채로 잘리면 클릭이 나므로 6ms 테이퍼를 건다
        k = min(int(0.006 * sr), len(seg) // 2)
        if k > 0:
            seg[-k:] *= np.linspace(1.0, 0.0, k)
        out[s:e] += seg * gain
    return out


def pad_to(x, dur, sr=SR):
    n = int(round(dur * sr))
    if x.ndim == 2:
        if x.shape[1] >= n:
            return x[:, :n]
        return np.pad(x, ((0, 0), (0, n - x.shape[1])))
    if len(x) >= n:
        return x[:n]
    return np.pad(x, (0, n - len(x)))


def fade(x, in_ms=3.0, out_ms=15.0, sr=SR):
    """클릭 방지용 앞뒤 페이드. x: (n,) 또는 (2, n)."""
    n = x.shape[-1]
    a = min(int(in_ms * sr / 1000), n // 2)
    b = min(int(out_ms * sr / 1000), n // 2)
    g = np.ones(n)
    if a > 0:
        g[:a] = np.linspace(0, 1, a)
    if b > 0:
        g[-b:] = np.linspace(1, 0, b)
    return x * g


def trim_silence(x, thresh=0.002, tail_ms=30, sr=SR):
    mono = np.abs(x).max(axis=0) if x.ndim == 2 else np.abs(x)
    idx = np.where(mono > thresh)[0]
    if len(idx) == 0:
        return x
    end = min(idx[-1] + int(tail_ms * sr / 1000), x.shape[-1])
    return x[..., :end]


def make_loop(x, xfade=1.5, sr=SR):
    """정상(stationary) 신호를 끝-처음 크로스페이드로 이음새 없는 루프로 만든다.
    x 는 원하는 루프 길이 + xfade 초 만큼 길게 만들어 넘긴다. x: (n,) 또는 (2, n)."""
    # 저역 DC/초저주파 드리프트는 루프 이음새에서 툭 튀므로 먼저 제거한다
    sos = signal.butter(1, 18, btype="high", fs=sr, output="sos")
    x = signal.sosfilt(sos, x, axis=-1)
    xf = int(xfade * sr)
    n = x.shape[-1] - xf
    head = x[..., :xf]
    tail = x[..., n:n + xf]
    w = np.linspace(0, np.pi / 2, xf)
    blended = head * np.sin(w) + tail * np.cos(w)
    body = x[..., :n].copy()
    body[..., :xf] = blended
    return body


def normalize(x, peak=0.9):
    m = np.max(np.abs(x))
    return x * (peak / m) if m > 1e-9 else x


def write_wav(path, x, sr=SR):
    """x: (n,) 모노 또는 (2, n) 스테레오, 값 범위 [-1, 1]. 16비트 PCM 으로 저장."""
    x = np.clip(x, -1.0, 1.0)
    if x.ndim == 2:
        inter = np.stack([x[0], x[1]], axis=1).reshape(-1)
        ch = 2
    else:
        inter = x
        ch = 1
    pcm = (inter * 32767).astype("<i2")
    with wave.open(path, "wb") as w:
        w.setnchannels(ch)
        w.setsampwidth(2)
        w.setframerate(sr)
        w.writeframes(pcm.tobytes())


# ---------------------------------------------------------------- 고급 프리미티브
def bell_partials(f0, ratios, taus, amps, dur, sr=SR, attack=0.001):
    """비조화 부분음의 합 = 종/금속/동전/모루 음색."""
    n = int(round(dur * sr))
    out = np.zeros(n)
    for r, tau, a in zip(ratios, taus, amps):
        out += a * np.sin(2 * np.pi * f0 * r * np.arange(n) / sr) * env_exp(n, tau, attack, sr)
    return out


def fm_bell(f0, dur, ratio=3.5, index=4.0, tau=0.5, sr=SR):
    """FM 종 (부드러운 차임). 모듈레이션 인덱스는 시간에 따라 감쇠 -> 어택은 밝고 여운은 맑다."""
    n = int(round(dur * sr))
    t = np.arange(n) / sr
    idx = index * np.exp(-t / (tau * 0.5))
    mod = np.sin(2 * np.pi * f0 * ratio * t)
    return np.sin(2 * np.pi * f0 * t + idx * mod) * env_exp(n, tau, 0.002, sr)


def pluck(f0, dur, tau=0.25, bright=0.5, sr=SR):
    """마림바/우드차임 계열: 기음 + 빠르게 죽는 배음."""
    n = int(round(dur * sr))
    t = np.arange(n) / sr
    out = np.sin(2 * np.pi * f0 * t) * env_exp(n, tau, 0.001, sr)
    out += bright * 0.5 * np.sin(2 * np.pi * f0 * 4 * t) * env_exp(n, tau * 0.25, 0.001, sr)
    out += bright * 0.25 * np.sin(2 * np.pi * f0 * 9.2 * t) * env_exp(n, tau * 0.12, 0.001, sr)
    return out


def thump(f_start, f_end, dur, tau, click=0.2, sr=SR, rng=None):
    """저음 타격 (펀치/발소리/쾅): 피치 하강 사인 + 짧은 클릭."""
    n = int(round(dur * sr))
    body = chirp(f_start, f_end, dur, sr) * env_exp(n, tau, 0.001, sr)
    if click > 0:
        r = rng if rng is not None else np.random.default_rng(3)
        c = lp(white(0.012, r, sr), 3500, 2, sr) * env_exp(int(0.012 * sr), 0.003, 0.0002, sr)
        body[:len(c)] += click * c
    return body


def noise_burst(dur, lo, hi, tau, rng, order=2, sr=SR, attack=0.001):
    n = int(round(dur * sr))
    return bp(white(dur, rng, sr), lo, hi, order, sr) * env_exp(n, tau, attack, sr)


def whoosh(dur, f_start, f_end, q=2.0, peak_at=0.4, rng=None, sr=SR, power=1.6):
    r = rng if rng is not None else np.random.default_rng(5)
    n = int(round(dur * sr))
    x = swept_bp(white(dur, r, sr), f_start, f_end, q=q, sr=sr)
    return x * env_bell(n, peak_at, power)


# 모음 포먼트 (Hz, 대역폭 Hz, 상대 게인). 몬스터 울음/으르렁 소스용
VOWELS = {
    "a": [(730, 90, 1.0), (1090, 110, 0.55), (2440, 140, 0.25)],
    "o": [(570, 80, 1.0), (840, 90, 0.45), (2410, 130, 0.15)],
    "u": [(300, 70, 1.0), (870, 90, 0.30), (2240, 120, 0.10)],
    "i": [(270, 60, 0.8), (2290, 120, 0.9), (3010, 150, 0.4)],
    "e": [(530, 80, 1.0), (1840, 110, 0.5), (2480, 140, 0.2)],
    "ae": [(660, 90, 1.0), (1720, 110, 0.55), (2410, 140, 0.25)],
}


def formant_filter(src, vowel, sr=SR):
    out = np.zeros_like(src)
    for f, bw, g in VOWELS[vowel]:
        out += g * resonator(src, f, q=f / bw, sr=sr)
    return out


def creature_voice(f0_curve, dur, vowel_a="a", vowel_b=None, growl_hz=0.0, growl_depth=0.0,
                   breath=0.1, jitter=0.012, rng=None, sr=SR, drive_amt=1.5):
    """성문 소스(톱니파) -> 포먼트 필터 = 크리처 음성. f0_curve: 피치 배열/스칼라.
    growl_hz>0 이면 진폭 변조로 으르렁 질감, vowel_b 가 있으면 a->b 로 모음이 변한다."""
    r = rng if rng is not None else np.random.default_rng(11)
    n = int(round(dur * sr))
    f0 = np.full(n, float(f0_curve)) if np.isscalar(f0_curve) else np.interp(
        np.linspace(0, 1, n), np.linspace(0, 1, len(f0_curve)), f0_curve)
    f0 = f0 * (1 + jitter * (smooth_noise(n, 25, r, sr) - 0.5) * 2) * (
        1 + 0.012 * np.sin(2 * np.pi * 5.5 * np.arange(n) / sr))
    src = saw_bl(f0, dur, sr, max_hz=5000)
    src = src + breath * white(dur, r, sr) * 0.4
    if growl_hz > 0:
        t = np.arange(n) / sr
        am = 1 - growl_depth * (0.5 + 0.5 * np.sin(2 * np.pi * growl_hz * t + 0.6 * smooth_noise(n, 8, r, sr)))
        src = src * am
    va = formant_filter(src, vowel_a, sr)
    if vowel_b:
        vb = formant_filter(src, vowel_b, sr)
        w = np.linspace(0, 1, n)
        out = va * (1 - w) + vb * w
    else:
        out = va
    return drive(out / (np.max(np.abs(out)) + 1e-9), drive_amt)


# ---------------------------------------------------------------- 중세/어두운 톤용 프리미티브
def karplus(f0, dur, decay=0.996, damp=0.5, rng=None, sr=SR):
    """카플러스-스트롱 발현악기(류트/하프) 뜯는 소리. damp 가 클수록 고음이 빨리 죽어 둔탁해진다."""
    r = rng if rng is not None else np.random.default_rng(7)
    n = int(round(dur * sr))
    period = max(int(round(sr / f0)), 2)
    buf = r.uniform(-1, 1, period)
    buf = lp(buf, 3500, 1, sr) if damp > 0 else buf
    out = np.zeros(n + period)
    out[:period] = buf
    pos = period
    prev = buf
    while pos < n:
        nxt = decay * 0.5 * (prev + np.roll(prev, 1))
        if damp > 0.4:
            nxt = 0.5 * (nxt + np.roll(nxt, 1))  # 한 번 더 평균내어 고음을 더 빨리 죽인다 (둔탁한 소리)
        m = min(period, len(out) - pos)
        out[pos:pos + m] = nxt[:m]
        prev = nxt
        pos += period
    out = out[:n]
    a = max(int(0.002 * sr), 1)
    out[:a] *= np.linspace(0, 1, a)
    return out


def church_bell(f0, dur, sr=SR, hum=1.0):
    """교회/성문 종: 낮은 '험'(0.5배) + 단3도(1.2배) 성분이 있어 음울하게 들린다."""
    ratios = [0.5, 1.0, 1.2, 1.5, 2.0, 2.5, 3.0, 4.2]
    taus = [1.6, 1.2, 0.9, 0.7, 0.5, 0.35, 0.25, 0.15]
    amps = [0.8 * hum, 1.0, 0.7, 0.45, 0.5, 0.25, 0.18, 0.1]
    x = bell_partials(f0, ratios, [t * dur / 2.0 for t in taus], amps, dur, sr, attack=0.002)
    return x


def horn(f, dur, cutoff=1300.0, vib=0.004, a=0.18, r=0.5, sr=SR):
    """저역 금관(호른/트롬본 느낌): 톱니파 + 로우패스, 어택이 느리고 비브라토가 얕다."""
    n = int(round(dur * sr))
    t = np.arange(n) / sr
    ff = f * (1 + vib * np.sin(2 * np.pi * 5.0 * t) * np.clip(t * 1.5, 0, 1))
    x = saw_bl(ff, dur, sr, max_hz=4000)
    x = lp(x, cutoff, 2, sr)
    return x * env_adsr(n, a, 0.1, 0.8, min(r, dur * 0.5), sr)


def choir_pad(freqs, dur, vowel="o", sr=SR, rng=None, detune=0.004):
    """합창단 같은 패드: 살짝 디튠된 톱니파 여러 개를 모음 포먼트로 거른다."""
    r = rng if rng is not None else np.random.default_rng(9)
    n = int(round(dur * sr))
    out = np.zeros(n)
    for f in freqs:
        for d in (-detune, 0.0, detune):
            out += saw_bl(f * (1 + d) * (1 + 0.003 * (smooth_noise(n, 0.3, r, sr) - 0.5)), dur, sr, max_hz=3500)
    out = formant_filter(out, vowel, sr)
    return out / (np.max(np.abs(out)) + 1e-9)


def timpani(dur=1.4, f=80.0, rng=None, sr=SR):
    r = rng if rng is not None else np.random.default_rng(13)
    n = int(round(dur * sr))
    body = chirp(f * 1.5, f, dur, sr) * env_exp(n, dur * 0.28, 0.002, sr)
    skin = bp(white(dur, r, sr), 120, 700, 2, sr) * env_exp(n, 0.06, 0.001, sr)
    return body + 0.5 * skin


def crow_caw(f0=520.0, dur=0.38, rng=None, sr=SR, rough=0.5):
    """까마귀: 거친 톱니파를 '아' 포먼트로 거르고 음정이 내려간다."""
    r = rng if rng is not None else np.random.default_rng(15)
    n = int(round(dur * sr))
    f = np.geomspace(f0 * 1.12, f0 * 0.72, n) * (1 + 0.04 * (smooth_noise(n, 60, r, sr) - 0.5))
    src = saw_bl(f, dur, sr, max_hz=5000)
    am = 1 - rough * (0.5 + 0.5 * np.sin(2 * np.pi * 70 * np.arange(n) / sr))
    x = formant_filter(src * am, "a", sr) + 0.15 * bp(white(dur, r, sr), 1500, 4500, 2, sr)
    x = x / (np.max(np.abs(x)) + 1e-9)
    return x * env_adsr(n, 0.02, 0.08, 0.7, 0.15, sr)
