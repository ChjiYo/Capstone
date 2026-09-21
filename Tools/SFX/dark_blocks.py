# ====================================================================== 어두운 중세 톤 (2차 개정)
# 스토리: 나르바쉬가 마을을 불태운 뒤의 복수극. 밝은 차임/스파클/고음 아르페지오 대신
#  - D 단조(D2 73.4 / A2 110 / D3 146.8 / F3 174.6 / A3 220 / D4 293.7), 완전5도·단3도 위주
#  - 류트 뜯는 소리, 교회 종(낮은 험 + 단3도), 호른, 합창 패드, 팀파니, 철/나무/가죽/양피지 질감
#  - 고역은 잘라내고(로우패스) 리버브 감쇠를 어둡게
# 이 블록은 make_sfx.py 안에서 같은 이름의 예전 레시피를 대체한다.

# ---------------------------------------------------------------- UI
@sfx("SFX_UI_Click", peak=0.55)
def _(rng):
    n = ns(0.10)
    knock = chirp(640, 470, 0.10) * env_exp(n, 0.020, 0.001)
    body = 0.5 * sine(1180, 0.10) * env_exp(n, 0.008, 0.001)
    tick = noise_burst(0.02, 500, 3200, 0.006, rng) * 0.6
    out = knock + body
    out[:len(tick)] += tick
    return out


@sfx("SFX_UI_Hover", peak=0.3)
def _(rng):
    n = ns(0.06)
    out = sine(880, 0.06) * env_exp(n, 0.012, 0.001)
    tick = noise_burst(0.03, 400, 2200, 0.008, rng)
    out[:len(tick)] += 0.4 * tick
    return out


@sfx("SFX_UI_Open", peak=0.55, stereo=True)
def _(rng):
    d = 0.7
    x = mix_at([
        (0.0, whoosh(0.4, 120, 1100, q=1.0, peak_at=0.6, rng=rng), 0.45),
        (0.03, karplus(146.8, 0.6, 0.997, 0.5, rng), 0.9),
        (0.11, karplus(220.0, 0.6, 0.997, 0.5, rng), 0.8),
    ], d)
    return rev(x, 0.6, 0.22, seed=2, damp=2800)


@sfx("SFX_UI_Close", peak=0.5, stereo=True)
def _(rng):
    d = 0.6
    x = mix_at([
        (0.0, whoosh(0.3, 1000, 120, q=1.0, peak_at=0.3, rng=rng), 0.45),
        (0.0, karplus(220.0, 0.5, 0.996, 0.5, rng), 0.8),
        (0.09, karplus(146.8, 0.5, 0.996, 0.5, rng), 0.9),
    ], d)
    return rev(x, 0.5, 0.2, seed=3, damp=2800)


@sfx("SFX_UI_Deny", peak=0.6)
def _(rng):
    def dull(f):
        n = ns(0.13)
        return (sine(f, 0.13) + 0.8 * sine(f * 1.06, 0.13)) * env_exp(n, 0.045, 0.002)   # 반음 부딪힘 = 불협
    x = mix_at([(0.0, dull(120), 1.0), (0.11, dull(95), 1.0),
                (0.0, thump(110, 60, 0.12, 0.04, 0.2, rng=rng), 0.6),
                (0.0, noise_burst(0.03, 300, 1500, 0.01, rng), 0.4)], 0.3)
    return fade(x)


@sfx("SFX_UI_Confirm", peak=0.6, stereo=True)
def _(rng):
    x = mix_at([
        (0.0, karplus(146.8, 1.0, 0.998, 0.4, rng), 0.9),
        (0.07, karplus(220.0, 1.0, 0.998, 0.4, rng), 0.8),
        (0.14, karplus(293.7, 1.0, 0.997, 0.5, rng), 0.7),
        (0.0, church_bell(110.0, 1.6), 0.25),
    ], 1.6)
    return rev(x, 1.0, 0.25, seed=4, damp=2800)


@variants("SFX_UI_DialogueBlip", 3, peak=0.3)
def _(rng, i):
    f = [230.0, 275.0, 330.0][i]
    n = ns(0.06)
    x = lp(square_bl(np.linspace(f, f * 0.9, n), 0.06, max_hz=2000), 850, 2)
    return fade(x * env_exp(n, 0.016, 0.001), 1, 10)


@sfx("SFX_UI_DialogueNext", peak=0.4)
def _(rng):
    n = ns(0.16)
    a = noise_burst(0.07, 700, 3200, 0.025, rng) * 0.6
    out = np.zeros(n)
    out[:len(a)] += a
    out += 0.6 * sine(300, 0.16)[:n] * env_exp(n, 0.04, 0.001)
    return fade(out)


@sfx("SFX_UI_DialogueOpen", peak=0.45, stereo=True)
def _(rng):
    d = 0.9
    n = ns(0.8)
    swell = (sine(110, 0.8) + 0.6 * sine(165, 0.8)) * env_bell(n, 0.5, 1.3) * 0.5
    x = mix_at([(0.0, swell, 1.0), (0.05, karplus(146.8, 0.8, 0.997, 0.5, rng), 0.7)], d)
    return rev(x, 0.8, 0.28, seed=5, damp=2500)


@sfx("SFX_UI_Coin", peak=0.6, stereo=True)
def _(rng):
    def clink(f0):
        b = bell_partials(f0, [1, 2.4, 4.1], [0.09, 0.05, 0.03], [1, 0.5, 0.25], 0.4)
        c = np.zeros_like(b)
        cc = noise_burst(0.01, 1500, 6000, 0.004, rng)
        c[:len(cc)] = cc
        return hp(b + 0.3 * c, 400)
    x = mix_at([(0.0, clink(1500), 0.9), (0.065, clink(1230), 0.7)], 0.55)
    return rev(x, 0.5, 0.15, seed=6, damp=3000)


@sfx("SFX_UI_Sell", peak=0.6, stereo=True)
def _(rng):
    parts = []
    for k, o in enumerate([0.0, 0.06, 0.1, 0.17, 0.22, 0.3]):
        f = rng.uniform(1000, 1900)
        b = bell_partials(f, [1, 2.4, 4.1], [0.08, 0.04, 0.02], [1, 0.5, 0.25], 0.3)
        parts.append((o, hp(b, 400), 0.85 - 0.09 * k))
    rustle = bp(white(0.4, rng), 400, 2600) * env_bell(ns(0.4), 0.3) * 0.25
    parts.append((0.0, rustle, 1.0))
    parts.append((0.0, thump(160, 90, 0.12, 0.04, 0.2, rng=rng), 0.4))
    return rev(mix_at(parts, 0.9), 0.5, 0.18, seed=7, damp=3000)


@sfx("SFX_UI_ItemPickup", peak=0.55, stereo=True)
def _(rng):
    n = ns(0.3)
    rustle = bp(white(0.3, rng), 300, 2400) * (0.4 + 0.6 * smooth_noise(n, 30, rng)) * env_bell(n, 0.35)
    x = mix_at([
        (0.0, rustle, 0.6),
        (0.05, thump(200, 110, 0.14, 0.05, 0.2, rng=rng), 0.5),
        (0.13, hp(bell_partials(1350, [1, 2.4, 4.1], [0.1, 0.05, 0.03], [1, 0.5, 0.25], 0.3), 500), 0.5),
    ], 0.5)
    return rev(x, 0.4, 0.15, seed=8, damp=2800)


@sfx("SFX_UI_ItemDrop", peak=0.6)
def _(rng):
    ting = hp(bell_partials(1600, [1, 2.4, 4.1], [0.12, 0.06, 0.03], [1, 0.5, 0.25], 0.4), 500)
    return mix_at([(0.0, thump(170, 75, 0.2, 0.06, 0.3, rng=rng), 0.8), (0.03, ting, 0.6),
                   (0.0, noise_burst(0.1, 300, 2000, 0.03, rng), 0.3)], 0.5)


@sfx("SFX_UI_Potion", peak=0.6, stereo=True)
def _(rng):
    def glug(f0, f1, d):
        n = ns(d)
        t = np.arange(n) / SR
        return chirp(f0, f1, d) * env_adsr(n, 0.006, 0.03, 0.6, 0.03) * (1 + 0.4 * np.sin(2 * np.pi * 30 * t))
    parts = [(0.12, glug(200, 380, 0.1), 0.9), (0.28, glug(180, 350, 0.11), 0.85), (0.44, glug(210, 420, 0.12), 0.8)]
    parts.append((0.0, chirp(300, 700, 0.05) * env_exp(ns(0.05), 0.02, 0.001), 0.5))   # 코르크 마개
    parts.append((0.0, noise_burst(0.05, 500, 2500, 0.015, rng), 0.4))
    parts.append((0.1, noise_burst(0.5, 200, 1100, 0.3, rng, attack=0.02), 0.2))
    parts.append((0.6, sine(146.8, 0.6) * env_bell(ns(0.6), 0.3, 1.2), 0.3))          # 낮고 따뜻한 여운
    return rev(mix_at(parts, 1.3), 0.6, 0.18, seed=9, damp=2600)


@sfx("SFX_UI_Enhance", peak=0.85, stereo=True)
def _(rng):
    def anvil(f0, amp):
        b = bell_partials(f0, [1, 2.32, 4.25, 6.63], [0.4, 0.25, 0.12, 0.07], [1, 0.8, 0.55, 0.3], 0.7)
        c = noise_burst(0.05, 1000, 6000, 0.02, rng)
        b[:len(c)] += 0.8 * c
        b[:ns(0.1)] += thump(140, 70, 0.1, 0.03, 0.0, rng=rng)
        return b * amp
    parts = [(0.0, anvil(520, 1.0), 0.9), (0.36, anvil(540, 0.85), 0.9), (0.72, anvil(500, 1.0), 1.0)]
    # 대장간 열기: 불꽃 튀는 소리 + 낮은 울림
    parts.append((0.7, hp(noise_burst(0.9, 1500, 6000, 0.35, rng), 1500) * 0.12, 1.0))
    parts.append((0.72, church_bell(98.0, 2.0), 0.35))
    parts.append((0.72, thump(90, 40, 0.5, 0.2, 0.2, rng=rng), 0.5))
    return rev(mix_at(parts, 2.6), 1.2, 0.28, seed=10, damp=2800)


@sfx("SFX_UI_QuestAccept", peak=0.6, stereo=True)
def _(rng):
    n = ns(0.3)
    flutter = 0.4 + 0.6 * smooth_noise(n, 30, rng)
    rustle = bp(white(0.3, rng), 500, 3500) * flutter * env_bell(n, 0.4)
    x = mix_at([
        (0.0, rustle, 0.5),
        (0.28, thump(130, 65, 0.25, 0.08, 0.3, rng=rng), 0.7),         # 봉인 도장 찍는 소리
        (0.34, karplus(146.8, 1.2, 0.998, 0.4, rng), 0.9),
        (0.44, karplus(220.0, 1.2, 0.998, 0.4, rng), 0.8),
        (0.3, church_bell(98.0, 1.6), 0.3),
    ], 1.8)
    return rev(x, 1.0, 0.22, seed=11, damp=2800)


@sfx("SFX_UI_QuestUpdate", peak=0.45, stereo=True)
def _(rng):
    x = mix_at([(0.0, church_bell(196.0, 0.9), 0.5), (0.0, thump(200, 110, 0.1, 0.03, 0.0, rng=rng), 0.4),
                (0.0, karplus(196.0, 0.5, 0.996, 0.5, rng), 0.6)], 0.9)
    return rev(x, 0.6, 0.2, seed=12, damp=2800)


@sfx("SFX_UI_LevelUp", peak=0.8, stereo=True)
def _(rng):
    d = 3.6
    parts = [
        (0.0, timpani(1.6, 80.0, rng), 0.9),
        (0.05, church_bell(146.8, 3.0), 0.4),
        (0.15, horn(146.8, 1.2, 1100, a=0.25), 0.55),
        (0.5, horn(220.0, 1.0, 1300, a=0.2), 0.5),
        (0.85, horn(293.7, 2.2, 1500, a=0.25, r=1.0), 0.5),
        (0.85, horn(174.6, 2.2, 1100, a=0.25, r=1.0), 0.35),
        (0.0, whoosh(0.7, 120, 900, q=1.0, peak_at=0.8, rng=rng), 0.25),
    ]
    return rev(mix_at(parts, d), 1.8, 0.3, seed=13, damp=2600)


@sfx("SFX_UI_ChapterTitle", peak=0.85, stereo=True)
def _(rng):
    d = 5.0
    n = ns(d)
    gong = bell_partials(73.4, [1, 1.47, 2.09, 2.55, 3.02, 3.98], [2.8, 2.2, 1.7, 1.3, 1.0, 0.7], [1, 0.7, 0.6, 0.5, 0.4, 0.25], d, attack=0.004)
    pad = choir_pad([73.4, 110.0, 146.8], d, "o", rng=rng) * env_bell(n, 0.4, 1.3)
    parts = [(0.0, gong, 1.0), (0.0, noise_burst(0.15, 150, 2500, 0.05, rng), 0.5),
             (0.2, pad, 0.35), (0.0, timpani(2.0, 55.0, rng), 0.6),
             (0.0, church_bell(73.4, 4.0), 0.3)]
    return rev(mix_at(parts, d), 2.6, 0.38, seed=14, damp=2500)


@sfx("SFX_UI_Ending", peak=0.85, stereo=True)
def _(rng):
    d = 8.0
    n = ns(d)
    parts = [(0.0, timpani(2.0, 70.0, rng), 0.9), (1.1, timpani(2.4, 62.0, rng), 0.9)]
    # 쓸쓸한 호른: D 단조 -> 열린 5도로 남긴다 (복수는 끝났지만 잃은 것은 돌아오지 않는다)
    for t, f, dd in [(0.0, 146.8, 1.0), (1.0, 220.0, 1.2), (2.3, 261.6, 1.0)]:
        parts.append((t, horn(f, dd + 0.6, 1200, a=0.3, r=0.8), 0.5))
    for f in [73.4, 110.0, 146.8, 174.6, 220.0]:
        parts.append((3.3, horn(f, 4.2, 1000, a=0.6, r=2.4), 0.3))
    parts.append((3.3, choir_pad([146.8, 220.0, 293.7], 4.4, "o", rng=rng) * env_bell(ns(4.4), 0.45, 1.2), 0.3))
    for t in [1.1, 3.3, 5.6]:
        parts.append((t, church_bell(98.0, 3.0), 0.35))
    return rev(mix_at(parts, d), 2.6, 0.33, seed=15, damp=2400)


@sfx("SFX_UI_Death", peak=0.9, stereo=True)
def _(rng):
    d = 4.5
    n = ns(d)
    drone = lp(saw_bl(np.linspace(196, 40, n), d, max_hz=1200), 420, 2) * env_exp(n, 1.6, 0.05)
    parts = [(0.0, thump(70, 36, 0.6, 0.2, 0.1, rng=rng), 1.0), (0.45, thump(62, 32, 0.6, 0.22, 0.1, rng=rng), 0.8),
             (0.15, drone, 0.55), (0.1, whoosh(1.4, 2500, 150, q=1.0, peak_at=0.15, rng=rng), 0.35),
             (0.9, church_bell(73.4, 3.6), 0.55), (2.1, church_bell(69.3, 2.4), 0.3)]
    return rev(mix_at(parts, d), 2.6, 0.42, seed=16, damp=2200)


@sfx("SFX_UI_LockOn", peak=0.5)
def _(rng):
    latch = noise_burst(0.02, 1000, 5000, 0.006, rng)
    thud = thump(220, 130, 0.09, 0.03, 0.1, rng=rng)
    tone = chirp(450, 620, 0.07) * env_adsr(ns(0.07), 0.004, 0.02, 0.5, 0.03)
    return fade(mix_at([(0.0, latch, 0.7), (0.0, thud, 0.8), (0.03, tone, 0.35)], 0.16))


@sfx("SFX_UI_LockSwitch", peak=0.4)
def _(rng):
    return fade(mix_at([(0.0, noise_burst(0.015, 1000, 4500, 0.004, rng), 0.6), (0.0, thump(240, 160, 0.06, 0.02, 0.0, rng=rng), 0.6)], 0.09))


@sfx("SFX_UI_LockRelease", peak=0.4)
def _(rng):
    tone = chirp(520, 300, 0.09) * env_adsr(ns(0.09), 0.003, 0.03, 0.4, 0.04)
    return fade(mix_at([(0.0, noise_burst(0.015, 800, 3500, 0.005, rng), 0.5), (0.0, tone, 0.5),
                        (0.0, thump(180, 100, 0.08, 0.03, 0.0, rng=rng), 0.5)], 0.14))


@sfx("SFX_UI_SkillReady", peak=0.4, stereo=True)
def _(rng):
    x = mix_at([(0.0, church_bell(293.7, 0.6), 0.5), (0.0, karplus(293.7, 0.4, 0.995, 0.5, rng), 0.5)], 0.6)
    return rev(x, 0.5, 0.18, seed=17, damp=2600)


# ================================================================ 상호작용
@sfx("SFX_Interact_Prompt", peak=0.3, stereo=True)
def _(rng):
    n = ns(0.5)
    x = (sine(220, 0.5) + 0.6 * sine(330, 0.5)) * env_adsr(n, 0.08, 0.12, 0.3, 0.28)
    return rev(x, 0.6, 0.25, seed=18, damp=2500)


@sfx("SFX_Interact_Talk", peak=0.45, stereo=True)
def _(rng):
    x = mix_at([(0.0, karplus(146.8, 0.9, 0.997, 0.5, rng), 0.9), (0.12, karplus(220.0, 0.9, 0.997, 0.5, rng), 0.8),
                (0.0, noise_burst(0.25, 200, 1400, 0.1, rng, attack=0.04), 0.06)], 1.0)
    return rev(x, 0.6, 0.22, seed=19, damp=2600)


@sfx("SFX_Interact_Gather", peak=0.6)
def _(rng):
    n = ns(0.4)
    flutter = 0.3 + 0.7 * smooth_noise(n, 22, rng)
    rustle = bp(white(0.4, rng), 900, 4500) * flutter * env_bell(n, 0.3)
    snap = noise_burst(0.03, 600, 2200, 0.01, rng)
    pop = thump(240, 130, 0.12, 0.04, 0.1, rng=rng)
    return mix_at([(0.0, rustle, 0.7), (0.06, snap, 0.6), (0.34, pop, 0.5)], 0.6)


@sfx("SFX_Interact_Respawn", peak=0.3, stereo=True)
def _(rng):
    x = mix_at([(0.0, karplus(220.0, 0.7, 0.996, 0.5, rng), 0.7),
                (0.0, bp(white(0.4, rng), 500, 2500) * env_bell(ns(0.4), 0.4) * 0.1, 1.0)], 0.8)
    return rev(x, 0.8, 0.3, seed=20, damp=2400)


@sfx("SFX_Portal_Enter", peak=0.85, stereo=True)
def _(rng):
    d = 2.4
    riser = swept_bp(white(1.8, rng), 100, 3500, q=1.3) * env_bell(ns(1.8), 0.75, 1.4)
    low = chirp(40, 220, 1.8) * env_bell(ns(1.8), 0.8, 1.3)
    dis = (sine(146.8, 1.8) + sine(155.6, 1.8)) * env_bell(ns(1.8), 0.7, 1.3) * 0.35     # 불협 두 음
    parts = [(0.0, riser, 0.7), (0.0, low, 0.9), (0.0, dis, 0.7), (1.2, thump(110, 35, 0.9, 0.3, 0.2, rng=rng), 1.0),
             (1.2, church_bell(73.4, 1.4), 0.3)]
    return rev(mix_at(parts, d), 1.6, 0.32, seed=21, damp=2400)


# ================================================================== 플레이어
@variants("SFX_Player_ShieldHit", 2, peak=0.8)
def _(rng, i):
    f0 = [330.0, 400.0][i]
    ring = bell_partials(f0, [1, 2.76, 5.4, 8.9], [0.28, 0.14, 0.07, 0.03], [1, 0.6, 0.35, 0.2], 0.5)
    hum = (sine(110, 0.5) + 0.7 * sine(165, 0.5)) * env_exp(ns(0.5), 0.25, 0.005) * 0.3
    hit = noise_burst(0.04, 500, 4000, 0.012, rng)
    return fade(lp(mix_at([(0.0, ring, 1.0), (0.0, hum, 1.0), (0.0, hit, 0.5),
                           (0.0, thump(150, 80, 0.15, 0.05, 0.2, rng=rng), 0.5)], 0.5), 6500))


@sfx("SFX_Player_ShieldUp", peak=0.7, stereo=True)
def _(rng):
    d = 1.8
    n = ns(1.2)
    swell = np.zeros(n)
    rise = np.linspace(1.0, 1.2, n)
    for f, a in [(110, 1.0), (165, 0.8), (220, 0.6), (330, 0.35)]:
        swell += a * sine(f * rise, 1.2)
    swell *= env_bell(n, 0.8, 1.5) * 0.4
    grit = bp(white(1.2, rng), 200, 1800) * env_bell(n, 0.75, 1.5) * 0.1
    parts = [(0.0, swell, 1.0), (0.0, grit, 1.0), (0.85, church_bell(146.8, 1.0), 0.45), (0.85, thump(100, 55, 0.3, 0.1, 0.1, rng=rng), 0.5)]
    return rev(mix_at(parts, d), 1.2, 0.3, seed=22, damp=2500)


@sfx("SFX_Player_ShieldBreak", peak=0.85, stereo=True)
def _(rng):
    d = 1.2
    parts = []
    for k in range(20):
        nn = ns(rng.uniform(0.05, 0.14))
        b = sine(rng.uniform(1200, 4200), nn / SR) * env_exp(nn, rng.uniform(0.03, 0.07), 0.0005)
        parts.append((rng.uniform(0.0, 0.4), b, rng.uniform(0.15, 0.3)))
    parts += [(0.0, noise_burst(0.35, 600, 5000, 0.12, rng), 0.6), (0.0, whoosh(0.6, 2500, 200, q=1.0, peak_at=0.2, rng=rng), 0.4),
              (0.0, thump(110, 45, 0.5, 0.16, 0.3, rng=rng), 0.8)]
    return rev(mix_at(parts, d), 0.8, 0.25, seed=23, damp=2500)


@sfx("SFX_Player_SkillA", peak=0.85, stereo=True)
def _(rng):
    d = 1.8
    n = ns(1.1)
    rise = lp(saw_bl(np.geomspace(70, 300, n), 1.1, max_hz=3000), 1800, 2) * env_bell(n, 0.8, 1.4)
    fire = bp(white(1.1, rng), 300, 2800) * env_bell(n, 0.7, 1.5) * (0.5 + 0.5 * smooth_noise(n, 25, rng))
    scrape = hp(bell_partials(1100, [1, 2.3, 4.0], [0.3, 0.15, 0.06], [1, 0.6, 0.3], 0.5), 300)     # 검날 긁는 소리
    parts = [(0.0, rise, 0.7), (0.0, fire, 0.55), (0.9, scrape, 0.5), (0.9, thump(90, 40, 0.4, 0.14, 0.2, rng=rng), 0.7)]
    return rev(mix_at(parts, d), 0.9, 0.2, seed=24, damp=2500)


# ==================================================================== 몬스터 / 마법
@sfx("SFX_Mon_Dissolve", peak=0.5, stereo=True)
def _(rng):
    d = 1.6
    n = ns(d)
    ash = bp(white(d, rng), 500, 3500) * env_bell(n, 0.3, 1.5) * 0.3
    parts = [(0.0, ash, 1.0), (0.0, chirp(500, 110, d) * env_exp(n, 0.6, 0.05), 0.2)]
    for k in range(30):
        s = rng.uniform(0.0, d * 0.8)
        if rng.random() > np.exp(-((s - 0.4) ** 2) / 0.15):
            continue
        nn = ns(0.05)
        parts.append((s, sine(rng.uniform(900, 3200), 0.05) * env_exp(nn, 0.014, 0.0005), rng.uniform(0.08, 0.2)))   # 사그라드는 불씨
    return rev(mix_at(parts, d), 0.8, 0.22, seed=26, damp=2500)


@sfx("SFX_Magic_Cast", peak=0.7, stereo=True)
def _(rng):
    d = 1.0
    n = ns(0.7)
    rise = chirp(120, 520, 0.7) * env_bell(n, 0.8, 1.3)
    dis = (sine(233, 0.7) + sine(247, 0.7)) * env_bell(n, 0.7, 1.3) * 0.25
    crackle = bp(white(0.7, rng), 700, 4000) * env_bell(n, 0.7, 1.5) * (0.4 + 0.6 * smooth_noise(n, 35, rng)) * 0.3
    return rev(mix_at([(0.0, rise, 0.5), (0.0, dis, 0.6), (0.0, crackle, 1.0)], d), 0.7, 0.2, seed=28, damp=2500)


@sfx("SFX_Magic_Impact", peak=0.9)
def _(rng):
    d = 0.9
    parts = [(0.0, thump(150, 50, 0.5, 0.18, 0.4, rng=rng), 0.9), (0.0, noise_burst(0.4, 300, 5000, 0.12, rng), 0.7)]
    for k in range(8):
        parts.append((rng.uniform(0.02, 0.4), sine(rng.uniform(1200, 3500), 0.05) * env_exp(ns(0.05), 0.015, 0.0005), 0.15))
    return rev(mix_at(parts, d), 0.6, 0.2, seed=29, damp=2500).mean(axis=0)


# ================================================================== 환경음
@sfx("SFX_Amb_Dread_Loop", peak=0.5, sr=AMB_SR, stereo=True)
def _(rng):
    """마을/들판 아래에 깔리는 불길한 저역 패드. 소리가 거의 안 들려도 분위기만 어둡게 눌러준다."""
    loop, xf = 28.0, 2.0

    def one(r):
        d = loop + xf
        n = ns(d, AMB_SR)
        t = np.arange(n) / AMB_SR
        pad = np.zeros(n)
        for f, a, rate in [(73.4, 1.0, 0.05), (73.9, 0.9, 0.04), (110.0, 0.6, 0.06), (146.8, 0.4, 0.07), (174.6, 0.25, 0.05), (220.0, 0.15, 0.06)]:
            pad += a * np.sin(2 * np.pi * f * t + r.uniform(0, 6.28)) * (0.3 + 0.7 * smooth_noise(n, rate, r, AMB_SR))
        choir = choir_pad([146.8, 220.0], d, "o", sr=AMB_SR, rng=r) * (0.1 + 0.5 * smooth_noise(n, 0.06, r, AMB_SR)) * 0.5
        rumble = lp(brown(d, r, AMB_SR), 140, 2, AMB_SR) * 0.4
        return pad * 0.3 + choir + rumble
    x = _decorrelated(one, 400)
    return make_loop(x, xf, AMB_SR)


@variants("SFX_Amb_Crow", 4, peak=0.5)
def _(rng, i):
    if i == 0:   # 까악 한 번
        x = mix_at([(0.0, crow_caw(520, 0.4, rng), 1.0)], 0.6)
    elif i == 1:  # 까악 까악
        x = mix_at([(0.0, crow_caw(480, 0.32, rng), 1.0), (0.42, crow_caw(500, 0.34, rng), 0.85)], 0.85)
    elif i == 2:  # 멀리서 세 번
        x = mix_at([(0.0, crow_caw(560, 0.28, rng), 0.9), (0.36, crow_caw(540, 0.28, rng), 0.75), (0.7, crow_caw(520, 0.3, rng), 0.6)], 1.05)
    else:         # 낮은 부엉이 울음
        n = ns(0.5)
        hoot = (sine(np.linspace(240, 200, n), 0.5) + 0.3 * sine(np.linspace(480, 400, n), 0.5)) * env_adsr(n, 0.06, 0.1, 0.7, 0.2)
        x = mix_at([(0.0, hoot, 0.9), (0.7, hoot, 0.8)], 1.4)
    return trim_silence(fade(rev(x, 0.9, 0.22, seed=40 + i, damp=2500).mean(axis=0)))
