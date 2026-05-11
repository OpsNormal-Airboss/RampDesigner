#!/usr/bin/env python3
"""
Generate top-down schematic SVG silhouettes for the ARLD aircraft library.

Coordinate system: viewBox "0 0 {wingspan} {length}" in feet.
  x=0: left wingtip, x=wingspan: right wingtip
  y=0: nose, y=length: tail
"""
import math
import os

OUT = os.path.join(os.path.dirname(__file__), "..", "arld", "data", "library", "silhouettes")
os.makedirs(OUT, exist_ok=True)

# SVG style constants
FUSELAGE_FILL   = "#4a4a4a"
WING_FILL       = "#5e5e5e"
TAIL_FILL       = "#6a6a6a"
NACELLE_FILL    = "#3a3a3a"
PROP_STROKE     = "#999999"
ROTOR_STROKE    = "#888888"

def fmt(v):
    return f"{v:.3f}".rstrip("0").rstrip(".")

def polygon(points, fill, opacity=1.0):
    pts = " ".join(f"{fmt(x)},{fmt(y)}" for x, y in points)
    op = f' opacity="{opacity:.2f}"' if opacity < 1.0 else ""
    return f'  <polygon points="{pts}" fill="{fill}"{op}/>\n'

def ellipse(cx, cy, rx, ry, fill):
    return f'  <ellipse cx="{fmt(cx)}" cy="{fmt(cy)}" rx="{fmt(rx)}" ry="{fmt(ry)}" fill="{fill}"/>\n'

def circle(cx, cy, r, fill="none", stroke="#000", sw=0.5, dash=""):
    d = f' stroke-dasharray="{dash}"' if dash else ""
    return (f'  <circle cx="{fmt(cx)}" cy="{fmt(cy)}" r="{fmt(r)}"'
            f' fill="{fill}" stroke="{stroke}" stroke-width="{fmt(sw)}"{d}/>\n')

def path(d, fill, stroke="none", sw=0):
    sk = f' stroke="{stroke}" stroke-width="{fmt(sw)}"' if stroke != "none" else ""
    return f'  <path d="{d}" fill="{fill}"{sk}/>\n'

def fuselage_path(cx, fw, nose_y, tail_y, widest_y=None):
    """Simple symmetric fuselage: pointed nose, widest at widest_y, tapered tail."""
    if widest_y is None:
        widest_y = nose_y + (tail_y - nose_y) * 0.45
    hw = fw / 2
    d = (f"M {fmt(cx)},{fmt(nose_y)} "
         f"C {fmt(cx+hw*0.5)},{fmt(nose_y+(widest_y-nose_y)*0.3)} "
         f"{fmt(cx+hw)},{fmt(nose_y+(widest_y-nose_y)*0.7)} "
         f"{fmt(cx+hw)},{fmt(widest_y)} "
         f"L {fmt(cx+hw*0.85)},{fmt(tail_y)} "
         f"L {fmt(cx)},{fmt(tail_y)} "
         f"L {fmt(cx-hw*0.85)},{fmt(tail_y)} "
         f"L {fmt(cx-hw)},{fmt(widest_y)} "
         f"C {fmt(cx-hw)},{fmt(nose_y+(widest_y-nose_y)*0.7)} "
         f"{fmt(cx-hw*0.5)},{fmt(nose_y+(widest_y-nose_y)*0.3)} "
         f"{fmt(cx)},{fmt(nose_y)} Z")
    return d

def wing_pair(cx, ws, le_root, te_root, le_tip, te_tip, fill=WING_FILL):
    hw = ws / 2
    right = [(cx+0, le_root), (hw+cx, le_tip), (hw+cx, te_tip), (cx+0, te_root)]
    # replace cx+0 -> fuselage edge
    r_pts = [(cx+1.0, le_root), (ws, le_tip), (ws, te_tip), (cx+1.0, te_root)]
    l_pts = [(cx-1.0, le_root), (0, le_tip), (0, te_tip), (cx-1.0, te_root)]
    return polygon(r_pts, fill) + polygon(l_pts, fill)

def hstab_pair(cx, span, le, te, fill=TAIL_FILL):
    hs = span / 2
    r = [(cx+1.0, le), (cx+hs, le+(te-le)*0.2), (cx+hs, te-(te-le)*0.1), (cx+1.0, te)]
    l = [(cx-1.0, le), (cx-hs, le+(te-le)*0.2), (cx-hs, te-(te-le)*0.1), (cx-1.0, te)]
    return polygon(r, fill) + polygon(l, fill)

def svg_wrap(ws, length, body):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" '
            f'viewBox="0 0 {fmt(ws)} {fmt(length)}">\n'
            f'{body}'
            f'</svg>\n')

# ---------------------------------------------------------------------------
# Single-engine piston fighters
# ---------------------------------------------------------------------------
def se_prop_fighter(name, ws, length, fw, prop_r,
                    wing_le, wing_te, hstab_span, hstab_le, hstab_te):
    cx = ws / 2
    body = ""
    # Prop arc (dashed)
    body += circle(cx, prop_r * 0.9, prop_r, stroke=PROP_STROKE,
                   sw=max(ws*0.008, 0.3), dash=f"{ws*0.04},{ws*0.02}")
    # HStab
    body += hstab_pair(cx, hstab_span, hstab_le, hstab_te)
    # Fuselage
    body += path(fuselage_path(cx, fw, 0, length), FUSELAGE_FILL)
    # Wings (drawn on top of fuselage)
    root_x = fw / 2
    r_wing = [(cx+root_x, wing_le), (ws, wing_le+(wing_te-wing_le)*0.35),
              (ws, wing_te-(wing_te-wing_le)*0.3), (cx+root_x, wing_te)]
    l_wing = [(cx-root_x, wing_le), (0, wing_le+(wing_te-wing_le)*0.35),
              (0, wing_te-(wing_te-wing_le)*0.3), (cx-root_x, wing_te)]
    body += polygon(r_wing, WING_FILL)
    body += polygon(l_wing, WING_FILL)
    return svg_wrap(ws, length, body)

# Special case: Spitfire (elliptical wing approximated with bezier)
def spitfire_svg(ws, length):
    cx = ws / 2
    fw = 2.5
    prop_r = 5.0
    body = ""
    body += circle(cx, prop_r * 0.9, prop_r, stroke=PROP_STROKE,
                   sw=0.4, dash="1.5,0.8")
    body += hstab_pair(cx, 11.0, 26.0, 30.5)
    body += path(fuselage_path(cx, fw, 0, length), FUSELAGE_FILL)
    # Elliptical wing: use bezier curves
    # Right wing leading edge: concave curve outward then back
    rl_d = (f"M {fmt(cx+fw/2)},11 "
            f"C {fmt(cx+fw/2+4)},9 {fmt(cx+fw/2+12)},9 {fmt(ws)},14 "
            f"L {fmt(ws)},20 "
            f"C {fmt(cx+fw/2+12)},24 {fmt(cx+fw/2+4)},23 {fmt(cx+fw/2)},21 Z")
    ll_d = (f"M {fmt(cx-fw/2)},11 "
            f"C {fmt(cx-fw/2-4)},9 {fmt(cx-fw/2-12)},9 0,14 "
            f"L 0,20 "
            f"C {fmt(cx-fw/2-12)},24 {fmt(cx-fw/2-4)},23 {fmt(cx-fw/2)},21 Z")
    body += path(rl_d, WING_FILL)
    body += path(ll_d, WING_FILL)
    return svg_wrap(ws, length, body)

# ---------------------------------------------------------------------------
# Multi-engine piston bombers
# ---------------------------------------------------------------------------
def twin_prop_bomber(name, ws, length, fw, prop_r, eng_x,
                     wing_le, wing_te, hstab_span, hstab_le, hstab_te):
    cx = ws / 2
    root_x = fw / 2
    body = ""
    # Engine nacelles (extend from wing LE to behind TE)
    for sign in (+1, -1):
        ex = cx + sign * eng_x
        nac_w = fw * 0.6
        nac_l = (wing_te - wing_le) * 0.85
        # Prop arcs
        body += circle(ex, wing_le - prop_r * 0.5, prop_r, stroke=PROP_STROKE,
                       sw=max(ws*0.006, 0.3), dash=f"{ws*0.03},{ws*0.015}")
        body += ellipse(ex, wing_le + nac_l * 0.45, nac_w / 2, nac_l / 2, NACELLE_FILL)
    body += hstab_pair(cx, hstab_span, hstab_le, hstab_te)
    body += path(fuselage_path(cx, fw, 0, length), FUSELAGE_FILL)
    r_wing = [(cx+root_x, wing_le), (ws, wing_le+(wing_te-wing_le)*0.25),
              (ws, wing_te-(wing_te-wing_le)*0.2), (cx+root_x, wing_te)]
    l_wing = [(cx-root_x, wing_le), (0, wing_le+(wing_te-wing_le)*0.25),
              (0, wing_te-(wing_te-wing_le)*0.2), (cx-root_x, wing_te)]
    body += polygon(r_wing, WING_FILL)
    body += polygon(l_wing, WING_FILL)
    return svg_wrap(ws, length, body)

def b17_svg(ws, length):
    cx = ws / 2; fw = 8.0; prop_r = 6.5
    body = ""
    # 4 engine nacelles
    for sign, dist in [(+1, 11), (+1, 26), (-1, 11), (-1, 26)]:
        ex = cx + sign * dist
        body += circle(ex, 24.0, prop_r, stroke=PROP_STROKE, sw=0.5, dash="3,1.5")
        body += ellipse(ex, 37.0, 2.5, 11.0, NACELLE_FILL)
    body += hstab_pair(cx, 43.0, 62.0, 72.0)
    body += path(fuselage_path(cx, fw, 0, length, widest_y=35.0), FUSELAGE_FILL)
    r_wing = [(cx+fw/2, 26), (ws, 36), (ws, 48), (cx+fw/2, 48)]
    l_wing = [(cx-fw/2, 26), (0, 36), (0, 48), (cx-fw/2, 48)]
    body += polygon(r_wing, WING_FILL)
    body += polygon(l_wing, WING_FILL)
    return svg_wrap(ws, length, body)

# ---------------------------------------------------------------------------
# Swept-wing / jet fighters
# ---------------------------------------------------------------------------
def swept_jet(name, ws, length, fw,
              wing_le_root, wing_le_tip, wing_te_root, wing_te_tip,
              hstab_span, hstab_le, hstab_te,
              twin=False, lerx=False):
    cx = ws / 2
    root_x = fw / 2
    body = ""
    body += hstab_pair(cx, hstab_span, hstab_le, hstab_te)
    # Intakes (twin engines): bumps on the sides
    if twin:
        for sign in (+1, -1):
            body += ellipse(cx + sign * (root_x + 1.5),
                            wing_le_root + (wing_te_root - wing_le_root) * 0.3,
                            1.5, (wing_te_root - wing_le_root) * 0.4, NACELLE_FILL)
    # LERX (blended leading edge extension)
    if lerx:
        lerx_len = wing_le_root * 0.55
        lerx_r = [(cx+root_x*0.6, lerx_len), (ws, wing_le_tip),
                  (ws, wing_te_tip), (cx+root_x*0.6, wing_te_root)]
        lerx_l = [(cx-root_x*0.6, lerx_len), (0, wing_le_tip),
                  (0, wing_te_tip), (cx-root_x*0.6, wing_te_root)]
        body += polygon(lerx_r, WING_FILL)
        body += polygon(lerx_l, WING_FILL)
    else:
        r_wing = [(cx+root_x, wing_le_root), (ws, wing_le_tip),
                  (ws, wing_te_tip), (cx+root_x, wing_te_root)]
        l_wing = [(cx-root_x, wing_le_root), (0, wing_le_tip),
                  (0, wing_te_tip), (cx-root_x, wing_te_root)]
        body += polygon(r_wing, WING_FILL)
        body += polygon(l_wing, WING_FILL)
    body += path(fuselage_path(cx, fw, 0, length), FUSELAGE_FILL)
    return svg_wrap(ws, length, body)

def f22_svg(ws, length):
    """F-22 diamond planform with canted tails and large leading-edge root extensions."""
    cx = ws / 2; fw = 4.0
    body = ""
    # Canted vertical tails (small outboard shapes near TE)
    for sign in (+1, -1):
        body += polygon([(cx+sign*fw, 42), (cx+sign*9, 44), (cx+sign*9, 55), (cx+sign*fw, 52)],
                        TAIL_FILL)
    # All-moving horizontal tails
    body += hstab_pair(cx, 18.0, 48.0, 58.0)
    # Large delta-ish wings with forward-swept TE
    hw = ws / 2
    r_wing = [(cx+fw/2, 20), (ws, 36), (ws, 44), (cx+fw/2, 52)]
    l_wing = [(cx-fw/2, 20), (0, 36), (0, 44), (cx-fw/2, 52)]
    body += polygon(r_wing, WING_FILL)
    body += polygon(l_wing, WING_FILL)
    body += path(fuselage_path(cx, fw, 0, length), FUSELAGE_FILL)
    return svg_wrap(ws, length, body)

def b2_svg(ws, length):
    """B-2 flying wing — W trailing edge, no distinct fuselage."""
    cx = ws / 2
    # Outer wing panels (highly swept)
    r_outer = [(cx+8, 20), (ws, 29), (ws, 55), (cx+8, 58)]
    l_outer = [(cx-8, 20), (0, 29), (0, 55), (cx-8, 58)]
    # Center section (crew/engines)
    center_d = (f"M {fmt(cx)},0 "
                f"L {fmt(cx+8)},20 L {fmt(cx+8)},58 "
                f"C {fmt(cx+4)},{fmt(length)} {fmt(cx)},{fmt(length)} "
                f"{fmt(cx-4)},{fmt(length)} "
                f"C {fmt(cx)},{fmt(length)} "
                f"L {fmt(cx-8)},58 L {fmt(cx-8)},20 Z")
    # Exhaust bays
    for sign in (+1, -1):
        body_pre = ""
    body = polygon(r_outer, WING_FILL) + polygon(l_outer, WING_FILL)
    body += path(center_d, FUSELAGE_FILL)
    # Engine exhaust slots
    for sign, ex in [(+1, cx+5), (+1, cx+14), (-1, cx-5), (-1, cx-14)]:
        body += ellipse(ex if sign == 1 else (cx - (ex - cx)),
                        48, 1.5, 4.5, NACELLE_FILL)
    return svg_wrap(ws, length, body)

def c130_svg(ws, length):
    cx = ws / 2; fw = 12.0; prop_r = 7.0
    body = ""
    for sign, dist in [(+1, 14), (+1, 30), (-1, 14), (-1, 30)]:
        ex = cx + sign * dist
        body += circle(ex, 36.0, prop_r, stroke=PROP_STROKE, sw=0.5, dash="4,2")
        body += ellipse(ex, 47.0, 3.5, 10.0, NACELLE_FILL)
    body += hstab_pair(cx, 52.0, 84.0, 93.0)
    body += path(fuselage_path(cx, fw, 0, length, widest_y=50.0), FUSELAGE_FILL)
    r_wing = [(cx+fw/2, 36), (ws, 41), (ws, 57), (cx+fw/2, 58)]
    l_wing = [(cx-fw/2, 36), (0, 41), (0, 57), (cx-fw/2, 58)]
    body += polygon(r_wing, WING_FILL)
    body += polygon(l_wing, WING_FILL)
    return svg_wrap(ws, length, body)

def c17_svg(ws, length):
    cx = ws / 2; fw = 18.0
    body = ""
    # 4 turbofan nacelles
    for sign, dist in [(+1, 19), (+1, 43), (-1, 19), (-1, 43)]:
        ex = cx + sign * dist
        body += ellipse(ex, 72.0, 4.5, 12.0, NACELLE_FILL)
    # T-tail: large HStab at top of fin — show as wide stabilizer
    body += hstab_pair(cx, 65.0, 157.0, 169.0)
    body += path(fuselage_path(cx, fw, 0, length, widest_y=90.0), FUSELAGE_FILL)
    r_wing = [(cx+fw/2, 58), (ws, 68), (ws, 87), (cx+fw/2, 88)]
    l_wing = [(cx-fw/2, 58), (0, 68), (0, 87), (cx-fw/2, 88)]
    body += polygon(r_wing, WING_FILL)
    body += polygon(l_wing, WING_FILL)
    return svg_wrap(ws, length, body)

def b52_svg(ws, length):
    cx = ws / 2; fw = 12.0
    body = ""
    # 8 engines in 4 twin-pods (2 inboard, 2 outboard per side)
    for sign, dist in [(+1, 35), (+1, 65), (-1, 35), (-1, 65)]:
        ex = cx + sign * dist
        # Each pod has 2 engine nacelles side-by-side
        for off in (-3.5, +3.5):
            body += ellipse(ex + off, 82.0, 3.0, 9.0, NACELLE_FILL)
    body += hstab_pair(cx, 55.0, 140.0, 154.0)
    body += path(fuselage_path(cx, fw, 0, length, widest_y=78.0), FUSELAGE_FILL)
    # Highly swept wing
    r_wing = [(cx+fw/2, 48), (ws, 98), (ws, 106), (cx+fw/2, 80)]
    l_wing = [(cx-fw/2, 48), (0, 98), (0, 106), (cx-fw/2, 80)]
    body += polygon(r_wing, WING_FILL)
    body += polygon(l_wing, WING_FILL)
    return svg_wrap(ws, length, body)

def helicopter_svg(ws, length):
    cx = ws / 2  # ws = rotor diameter
    rotor_r = ws / 2
    fuselage_fw = 5.0
    body = ""
    # Main rotor disk
    body += circle(cx, rotor_r * 0.8, rotor_r, stroke=ROTOR_STROKE, sw=0.7,
                   dash=f"{ws*0.05},{ws*0.03}")
    # Tail boom
    body += path(fuselage_path(cx, fuselage_fw * 0.5, length * 0.55, length * 0.97,
                               widest_y=length * 0.7), TAIL_FILL)
    # Tail rotor
    body += circle(cx, length * 0.95, ws * 0.1, stroke=ROTOR_STROKE, sw=0.4,
                   dash=f"{ws*0.03},{ws*0.02}")
    # Main fuselage / cabin
    body += path(fuselage_path(cx, fuselage_fw, length * 0.12, length * 0.62,
                               widest_y=length * 0.35), FUSELAGE_FILL)
    # Mast-mounted sight (MMS) — small circle
    body += circle(cx, length * 0.15, ws * 0.05, fill=NACELLE_FILL)
    # Skids
    for sign in (+1, -1):
        body += polygon([(cx+sign*(fuselage_fw*0.7), length*0.28),
                         (cx+sign*(fuselage_fw*0.7+1.5), length*0.28),
                         (cx+sign*(fuselage_fw*0.7+1.5), length*0.62),
                         (cx+sign*(fuselage_fw*0.7), length*0.62)], NACELLE_FILL)
    return svg_wrap(ws, length, body)

def biplane_svg(ws, length):
    """Pitts S-2C: show upper wing (full span) and lower wing (shorter) with offset."""
    cx = ws / 2; fw = 1.8
    prop_r = 2.5
    lower_ws = ws * 0.85  # lower wing is shorter
    body = ""
    body += circle(cx, prop_r * 0.9, prop_r, stroke=PROP_STROKE, sw=0.35, dash="1,0.5")
    body += hstab_pair(cx, 6.0, 13.5, 17.5)
    body += path(fuselage_path(cx, fw, 0, length), FUSELAGE_FILL)
    # Lower wing (slightly behind upper, shorter)
    lw_root = fw / 2
    lower_hw = lower_ws / 2
    offset_x = cx - lower_hw
    r_lower = [(cx+lw_root, 7.5), (cx+lower_hw, 8.5), (cx+lower_hw, 12.5), (cx+lw_root, 12.5)]
    l_lower = [(cx-lw_root, 7.5), (cx-lower_hw, 8.5), (cx-lower_hw, 12.5), (cx-lw_root, 12.5)]
    body += polygon(r_lower, WING_FILL, opacity=0.75)
    body += polygon(l_lower, WING_FILL, opacity=0.75)
    # Upper wing (full span)
    r_upper = [(cx+lw_root, 6.0), (ws, 7.2), (ws, 11.5), (cx+lw_root, 11.0)]
    l_upper = [(cx-lw_root, 6.0), (0, 7.2), (0, 11.5), (cx-lw_root, 11.0)]
    body += polygon(r_upper, WING_FILL)
    body += polygon(l_upper, WING_FILL)
    return svg_wrap(ws, length, body)

# ---------------------------------------------------------------------------
# Generate all 20 aircraft SVGs
# ---------------------------------------------------------------------------
aircraft_svgs = {
    "north-american-p51-d": lambda: se_prop_fighter(
        "p51d", 37.0, 32.3, 2.8, 5.5,
        wing_le=12.5, wing_te=21.0,
        hstab_span=13.0, hstab_le=27.5, hstab_te=31.5),

    "supermarine-spitfire-ixc": lambda: spitfire_svg(36.9, 31.1),

    "grumman-f6f-5": lambda: se_prop_fighter(
        "f6f5", 42.9, 33.7, 3.0, 6.5,
        wing_le=12.0, wing_te=22.0,
        hstab_span=15.0, hstab_le=28.5, hstab_te=33.0),

    "curtiss-p40-n": lambda: se_prop_fighter(
        "p40n", 37.4, 33.4, 2.8, 5.5,
        wing_le=13.0, wing_te=22.5,
        hstab_span=13.5, hstab_le=28.0, hstab_te=33.0),

    "republic-p47-d": lambda: se_prop_fighter(
        "p47d", 40.9, 36.1, 4.0, 7.0,
        wing_le=13.5, wing_te=23.5,
        hstab_span=15.0, hstab_le=30.5, hstab_te=35.5),

    "boeing-b17-g": lambda: b17_svg(103.8, 74.9),

    "north-american-b25-j": lambda: twin_prop_bomber(
        "b25j", 67.6, 52.8, 6.0, 6.0, eng_x=12.0,
        wing_le=18.0, wing_te=34.0,
        hstab_span=28.0, hstab_le=44.0, hstab_te=52.0),

    "douglas-a26-c": lambda: twin_prop_bomber(
        "a26c", 70.0, 50.9, 5.5, 6.0, eng_x=13.0,
        wing_le=17.5, wing_te=33.5,
        hstab_span=26.0, hstab_le=42.0, hstab_te=50.0),

    "mcdonnell-douglas-f4-e": lambda: swept_jet(
        "f4e", 38.4, 63.0, 5.0,
        wing_le_root=28.0, wing_le_tip=37.0,
        wing_te_root=40.0, wing_te_tip=38.0,
        hstab_span=19.0, hstab_le=51.0, hstab_te=59.0,
        twin=True),

    "general-dynamics-f16-a": lambda: swept_jet(
        "f16a", 31.0, 49.3, 3.5,
        wing_le_root=24.0, wing_le_tip=30.0,
        wing_te_root=40.0, wing_te_tip=36.0,
        hstab_span=17.0, hstab_le=39.0, hstab_te=46.0,
        lerx=True),

    "mcdonnell-douglas-fa18-c": lambda: swept_jet(
        "fa18c", 40.4, 56.0, 4.0,
        wing_le_root=24.0, wing_le_tip=30.0,
        wing_te_root=40.0, wing_te_tip=36.0,
        hstab_span=18.0, hstab_le=46.0, hstab_te=54.0,
        twin=True, lerx=True),

    "lockheed-martin-f22-a": lambda: f22_svg(44.5, 62.1),

    "lockheed-martin-f35-a": lambda: swept_jet(
        "f35a", 35.0, 51.4, 3.8,
        wing_le_root=21.0, wing_le_tip=27.0,
        wing_te_root=40.0, wing_te_tip=35.0,
        hstab_span=15.0, hstab_le=41.0, hstab_te=48.5,
        lerx=True),

    "northrop-grumman-b2-a": lambda: b2_svg(172.0, 69.0),
    "lockheed-c130-h": lambda: c130_svg(132.6, 97.8),
    "boeing-c17-a": lambda: c17_svg(169.8, 174.1),
    "boeing-b52-h": lambda: b52_svg(185.0, 159.9),

    "extra-ea300-l": lambda: se_prop_fighter(
        "ea300l", 26.3, 23.3, 2.2, 3.0,
        wing_le=7.5, wing_te=14.5,
        hstab_span=8.0, hstab_le=18.5, hstab_te=22.5),

    "pitts-s2-c": lambda: biplane_svg(20.0, 17.9),
    "bell-oh58-d": lambda: helicopter_svg(35.0, 40.9),
}

# Fix B-2 body
def b2_svg_fixed(ws, length):
    cx = ws / 2
    r_outer = [(cx+8, 20), (ws, 29), (ws, 55), (cx+8, 58)]
    l_outer = [(cx-8, 20), (0, 29), (0, 55), (cx-8, 58)]
    center_d = (f"M {fmt(cx)},0 "
                f"L {fmt(cx+8)},20 L {fmt(cx+8)},58 "
                f"L {fmt(cx+4)},{fmt(length)} "
                f"L {fmt(cx-4)},{fmt(length)} "
                f"L {fmt(cx-8)},58 L {fmt(cx-8)},20 Z")
    body = polygon(r_outer, WING_FILL) + polygon(l_outer, WING_FILL)
    body += path(center_d, FUSELAGE_FILL)
    for ex in [cx+5, cx+14, cx-5, cx-14]:
        body += ellipse(ex, 48, 1.5, 4.5, NACELLE_FILL)
    return svg_wrap(ws, length, body)

aircraft_svgs["northrop-grumman-b2-a"] = lambda: b2_svg_fixed(172.0, 69.0)

count = 0
for aircraft_id, generator in aircraft_svgs.items():
    svg_path = os.path.join(OUT, f"{aircraft_id}.svg")
    with open(svg_path, "w") as f:
        f.write(generator())
    count += 1
    print(f"  Generated: {aircraft_id}.svg")

print(f"\n✓ Generated {count} SVG silhouettes in {OUT}")
