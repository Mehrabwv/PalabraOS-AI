"""PalabraOS Slim enclosure generator.  Run:  python3 make_enclosure_slim.py
Needs: pip install manifold3d trimesh numpy
Writes back_tray_slim.stl and front_panel_slim.stl (millimetres, already oriented for printing).

Slim is 10.5 mm thick (the default enclosure is 13 mm) with the same footprint. How:
  - The display module is turned end for end, so its 8 solder holes sit next to the buttons.
  - The XIAO lies crosswise under that end of the display, USB-C out of the top long edge.
    Display, button and battery wires are all short and none has to pass the battery.
  - The battery lies directly against the back of the display board, clear of the cable
    socket, so the 2 mm wire gap of the default design is gone.
  - Front panel 1.2 mm and floor 1.0 mm instead of 1.5 mm each.
Change the numbers in the PARAMETERS block and run again to resize anything."""
import json, sys
import numpy as np, trimesh
from manifold3d import Manifold, CrossSection

# ---------------- PARAMETERS (mm) ----------------
MOD_L, MOD_W, PCB_T = 72.0, 30.5, 3.0      # display module board (3 mm includes the parts on its back)
PANEL_L, PANEL_T    = 59.5, 1.0            # white panel on the board (centred along the board)
HOLE_INSET          = 2.75                 # hole centre to each board edge   <-- measure and adjust
PEG_D               = 2.6                  # peg for the 3 mm holes (0.2 mm slack each way)
SOCKET_U, SOCKET_T  = (44.0, 50.0), 2.0    # cable socket: position along the board, height on its back
XIAO_L, XIAO_W, XIAO_T = 21.0, 17.8, 1.2   # XIAO ESP32-S3 board
USB_H               = 3.2                  # USB-C socket height above the XIAO board
BAT_L, BAT_W, BAT_T = 35.0, 30.0, 5.0      # 503035 LiPo
BAT_POCKET          = 37.0                 # pocket length: cell plus its little protection board
SW_BODY             = 3.5                  # tactile switch body 6 x 6 x 3.5
PITCH               = 8.0                  # button spacing
WALL, FLOOR, PLATE  = 1.6, 1.0, 1.2
GAP, TAB, PADZONE   = 0.4, 1.5, 24.0
R                   = 3.0                  # outer corner radius
BAT_SLACK           = 0.3                  # breathing room above the battery
H = PLATE + PCB_T + BAT_SLACK + BAT_T + FLOOR   # total thickness = 10.5

# ---------------- derived ----------------
IN_L = TAB + GAP + MOD_L + GAP + PADZONE
IN_W = MOD_W + 2 * GAP
L, W = IN_L + 2 * WALL, IN_W + 2 * WALL
X0 = -IN_L / 2                       # inside face of the left wall
MOD0 = X0 + TAB + GAP                # left end of the module (its ribbon-cable end)
MODX = MOD0 + MOD_L / 2
PADX = MOD0 + MOD_L + GAP + PADZONE / 2
UNDER = H - PLATE                    # underside of the front panel = top of the walls
BOARD_Z = UNDER - PCB_T              # back face of the module board
PADS = {'U': (0, PITCH), 'D': (0, -PITCH), 'L': (-PITCH, 0), 'R': (PITCH, 0), 'C': (0, 0)}
XB = FLOOR + 0.7                     # underside of the XIAO board (raised for the battery pads)
XA = MOD0 + SOCKET_U[1] + 1.6        # XIAO: left long edge (it lies crosswise)
XC = XA + XIAO_W / 2                 # XIAO centre line = centre of the USB-C opening
YU = IN_W / 2 - 0.2                  # XIAO: USB end, against the top wall
YE = YU - XIAO_L                     # XIAO: inner end
BX0 = MOD0 + HOLE_INSET + PEG_D / 2 + 0.55   # battery pocket starts just past the left pegs
BX1 = BX0 + BAT_POCKET

def box(x0, x1, y0, y1, z0, z1):
    x0, x1 = sorted((x0, x1)); y0, y1 = sorted((y0, y1))
    return Manifold.cube([x1 - x0, y1 - y0, z1 - z0]).translate([x0, y0, z0])

def cyl(r, x, y, z0, z1, r_low=None):
    return Manifold.cylinder(z1 - z0, r_low if r_low else r, r, 48).translate([x, y, z0])

def rrect(w, h, r, z0, z1):
    a = np.linspace(0, np.pi / 2, 13)
    pts = []
    for cx, cy, a0 in ((w/2 - r, h/2 - r, 0), (-w/2 + r, h/2 - r, np.pi/2), (-w/2 + r, -h/2 + r, np.pi), (w/2 - r, -h/2 + r, 1.5*np.pi)):
        pts += [(cx + r * np.cos(t + a0), cy + r * np.sin(t + a0)) for t in a]
    return CrossSection([np.array(pts)]).extrude(z1 - z0).translate([0, 0, z0])

def union(parts):
    out = parts[0]
    for p in parts[1:]: out = out + p
    return out

# ================= BACK TRAY =================
def back_tray():
    t = rrect(L, W, R, 0, UNDER) - rrect(IN_L, IN_W, R - WALL, FLOOR, UNDER + 1)
    add, cut = [], []
    # USB-C opening in the top long wall
    cut.append(box(XC - 5.0, XC + 5.0, IN_W/2 - 0.01, W/2 + 1, XB + XIAO_T - 0.3, XB + XIAO_T + USB_H + 0.4))
    # XIAO cradle: a small ledge and fence at each corner (clear of the solder pads), a stop at the inner end
    for xe, s in ((XA, -1), (XA + XIAO_W, 1)):                 # the two long edges
        for y0, y1 in ((YU - 2.4, YU + 0.2), (YE, YE + 2.4)):  # the two ends of each edge
            add.append(box(xe - s * 0.8, xe + s * 0.2, y0, y1, FLOOR, XB))
            add.append(box(xe + s * 0.2, xe + s * 1.4, y0, y1, FLOOR, XB + XIAO_T + 0.4))
    add.append(box(XC - 5, XC + 5, YE, YE + 0.8, FLOOR, XB))
    add.append(box(XC - 5, XC + 5, YE - 1.4, YE - 0.2, FLOOR, XB + XIAO_T + 0.4))
    # battery end stops (the side walls hold its long sides); the gap in the middle is for its wires
    for x0, x1 in ((BX0 - 1.2, BX0), (BX1, BX1 + 1.2)):
        for s in (-1, 1):
            add.append(box(x0, x1, s * 6, s * 12, FLOOR, FLOOR + 3))
    # a post behind every switch
    for dx, dy in PADS.values():
        add.append(cyl(2.0, PADX + dx, dy, FLOOR, UNDER - SW_BODY - 0.3))
    # windows for the two snap clips, pockets for the two hook tabs
    for s in (-1, 1):
        cut.append(box(PADX - 3.5, PADX + 3.5, s * (IN_W/2 - 0.01), s * (W/2 + 1), UNDER - 5.3, UNDER - 3.6))
        cut.append(box(X0 - 0.9, X0 + 0.01, s * 10 - 3.1, s * 10 + 3.1, UNDER - 3.2, UNDER - 1.7))
    return t + union(add) - union(cut)

# ================= FRONT PANEL =================
def front_panel():
    p = rrect(L, W, R, UNDER, H)
    p -= box(MODX - PANEL_L/2 - 0.2, MODX + PANEL_L/2 + 0.2, -MOD_W/2 - 0.2, MOD_W/2 + 0.2, UNDER - 1, H + 1)
    add = []
    for dx, dy in PADS.values():
        p -= cyl(2.0, PADX + dx, dy, UNDER - 1, H + 1)                     # plunger holes
    # display pegs with a tapered tip
    for ex in (HOLE_INSET, MOD_L - HOLE_INSET):
        for s in (-1, 1):
            x, y = MOD0 + ex, s * (MOD_W/2 - HOLE_INSET)
            add.append(cyl(PEG_D/2, x, y, BOARD_Z, UNDER + 0.01))
            add.append(cyl(PEG_D/2, x, y, BOARD_Z - 0.6, BOARD_Z, r_low=PEG_D/2 - 0.4))
    # switch collars, trimmed to stay 0.2 mm clear of the walls
    collars = union([box(PADX + dx - 4, PADX + dx + 4, dy - 4, dy + 4, UNDER - 2.6, UNDER + 0.01) for dx, dy in PADS.values()])
    collars -= union([box(PADX + dx - 3.2, PADX + dx + 3.2, dy - 3.2, dy + 3.2, UNDER - 3, UNDER + 0.02) for dx, dy in PADS.values()])
    collars ^= box(X0 + 0.2, -X0 - 0.2, -IN_W/2 + 0.2, IN_W/2 - 0.2, 0, H)
    add.append(collars)
    for s in (-1, 1):
        # hook tab at the left end: goes in first, its lip slides into the wall pocket
        add.append(box(X0 + 0.15, X0 + 1.35, s * 10 - 2.8, s * 10 + 2.8, UNDER - 3.0, UNDER + 0.01))
        add.append(box(X0 - 0.75, X0 + 0.15, s * 10 - 2.8, s * 10 + 2.8, UNDER - 3.0, UNDER - 1.8))
        # snap clip beside the buttons: springy arm with a wedge-shaped barb
        y_face = s * (IN_W/2 - 0.15)
        add.append(box(PADX - 3, PADX + 3, y_face - s * 1.3, y_face, UNDER - 5.0, UNDER + 0.01))
        catch = box(PADX - 3, PADX + 3, y_face - s * 0.1, y_face + s * 0.8, UNDER - 3.95, UNDER - 3.75)
        tip = box(PADX - 3, PADX + 3, y_face - s * 0.1, y_face, UNDER - 5.0, UNDER - 4.95)
        add.append(Manifold.batch_hull([catch, tip]))
    return p + union(add)

# ---- the real parts as simple blocks: used to check that nothing collides, and by the 3D simulator ----
def part_boxes():
    b = []   # (group, colour, [x0, y0, z0, x1, y1, z1])
    b.append(('display', 'module', [MOD0, -MOD_W/2, BOARD_Z, MOD0 + MOD_L, MOD_W/2, UNDER]))
    b.append(('display', 'panel', [MODX - PANEL_L/2, -MOD_W/2, UNDER, MODX + PANEL_L/2, MOD_W/2, UNDER + PANEL_T]))
    b.append(('display', 'white', [MOD0 + SOCKET_U[0], -7.5, BOARD_Z - SOCKET_T, MOD0 + SOCKET_U[1], 7.5, BOARD_Z]))
    b.append(('board', 'black', [XA, YE, XB, XA + XIAO_W, YU, XB + XIAO_T]))
    b.append(('board', 'metal', [XC - 4.5, YU - 7.0, XB + XIAO_T, XC + 4.5, YU + 1.4, XB + XIAO_T + USB_H]))
    b.append(('board', 'metal', [XC - 6, YE + 1.5, XB + XIAO_T, XC + 6, YE + 12.5, XB + XIAO_T + 2.3]))
    b.append(('batt', 'metal', [BX0 + 0.5, -BAT_W/2, FLOOR, BX0 + 0.5 + BAT_L, BAT_W/2, FLOOR + BAT_T]))
    for dx, dy in PADS.values():
        b.append(('btn', 'metal', [PADX + dx - 3, dy - 3, UNDER - SW_BODY, PADX + dx + 3, dy + 3, UNDER]))
    return b

def wires():
    """Every wire as a list of points, for the simulator. XIAO seen from the front with USB at the top:
    left column of pads D0..D6, right column 5V GND 3V3 D10 D9 D8 D7 (top to bottom)."""
    left = ['D0', 'D1', 'D2', 'D3', 'D4', 'D5', 'D6']; right = ['5V', 'GND', '3V3', 'D10', 'D9', 'D8', 'D7']
    zp = XB + XIAO_T
    def pad(name):
        col, i = (left, left.index(name)) if name in left else (right, right.index(name))
        return (XA + 1.27 if col is left else XA + XIAO_W - 1.27), YU - 2.88 - 2.54 * i
    out = []
    # display: 2 x 4 solder holes at the button end of the module, straight down to the XIAO
    holes = {'BUSY': (70.5, 3.81), 'RES': (67.1, 3.81), 'DC': (70.5, 1.27), 'CS': (67.1, 1.27),
             'SCL': (70.5, -1.27), 'SDA': (67.1, -1.27), 'GND': (70.5, -3.81), 'VCC': (67.1, -3.81)}
    for sig, to, colour in (('BUSY', 'D4', 0x7d4fc4), ('RES', 'D5', 0xf2f2ee), ('DC', 'D6', 0xe8782a), ('CS', 'D9', 0x2f6fd0),
                            ('SCL', 'D8', 0x2f9e54), ('SDA', 'D10', 0xe6c52f), ('GND', 'GND', 0x222426), ('VCC', '3V3', 0xd23b3b)):
        hx, hy = MOD0 + holes[sig][0], holes[sig][1]; px, py = pad(to)
        out.append((f'Display {sig}', to, colour, [[hx, hy, BOARD_Z], [hx, hy, BOARD_Z - 0.9], [(hx + px) / 2, (hy + py) / 2, BOARD_Z - 1.4], [px, py, zp + 0.9], [px, py, zp]]))
    # buttons: the centre one goes straight to the near column; the other four go round the XIAO's inner end
    zl = FLOOR + 0.9
    for k, (name, to, colour) in enumerate((('U', 'D0', 0x39b8c9), ('D', 'D1', 0xc24fb0), ('L', 'D2', 0x9bcf3c), ('R', 'D3', 0xe89ab4))):
        sx, sy = PADX + PADS[name][0] - 3.3, PADS[name][1] - 2; px, py = pad(to)
        yr = YE - 2.6 - 1.1 * k; xl = XA - 2.2 - 1.0 * k
        out.append((f'Button {name}', to, colour, [[sx, sy, UNDER - SW_BODY + 0.4], [sx - 1.5, sy, zl + 1], [XA + XIAO_W + 3, yr, zl], [xl + 1.5, yr, zl], [xl, yr + 2, zl], [xl, py - 1, zl + 0.6], [px, py, zp + 0.6], [px, py, zp]]))
    sx, sy = PADX - 3.3, 2.0; px, py = pad('D7')
    out.append(('Button C', 'D7', 0x9a6a3c, [[sx, sy, UNDER - SW_BODY + 0.4], [sx - 2, sy, zl + 2], [px + 3, py, zp + 1.2], [px, py, zp]]))
    gz = UNDER - SW_BODY + 0.4; px, py = pad('GND')
    out.append(('Button ground', 'GND', 0x222426, [[PADX + 3.3, -PITCH - 2, gz], [PADX + PITCH + 3.3, -2, gz], [PADX + 3.3, -2, gz], [PADX + 3.3, PITCH - 2, gz],
                [PADX - PITCH + 3.3, -2.2, gz], [PADX - PITCH - 4.2, 3, zl + 2.5], [px + 3.5, py, zp + 1.6], [px, py, zp]]))
    # battery: under the cable socket, to the BAT pads on the underside of the XIAO
    for s, colour, name in ((1, 0xd23b3b, 'Battery +'), (-1, 0x222426, 'Battery -')):
        out.append((name, 'BAT' + ('+' if s > 0 else '-'), colour, [[BX0 + 0.5 + BAT_L, 2 * s, FLOOR + 2], [BX1 + 3, 2 * s, FLOOR + 0.5], [XA - 1, 4 + 2 * s, FLOOR + 0.4], [XA + 4, 4 + 2 * s, FLOOR + 0.4]]))
    return out

def to_trimesh(m):
    mesh = m.to_mesh()
    return trimesh.Trimesh(np.asarray(mesh.vert_properties)[:, :3], np.asarray(mesh.tri_verts), process=True)

if __name__ == '__main__':
    tray, panel = back_tray(), front_panel()
    print(f'outside size: {L:.1f} x {W:.1f} x {H:.1f} mm')
    ok = True
    for name, part in (('back_tray_slim', tray), ('front_panel_slim', panel)):
        out = part if name.startswith('back') else part.rotate([180, 0, 0])      # panel prints face down
        tm = to_trimesh(out); tm.apply_translation([0, 0, -tm.bounds[0][2]])
        tm.export(name + '.stl')
        size = tm.bounds[1] - tm.bounds[0]
        pieces = len(part.decompose())
        print(f'{name}.stl: {size[0]:.1f} x {size[1]:.1f} x {size[2]:.1f} mm, {tm.volume/1000:.1f} cm3, watertight={tm.is_watertight}, pieces={pieces}')
        ok &= tm.is_watertight and pieces == 1
    v = (tray ^ panel).volume(); ok &= abs(v) < 1e-3
    print(f'tray vs panel overlap: {abs(v):.3f} mm3')
    for group, colour, (x0, y0, z0, x1, y1, z1) in part_boxes():
        solid = box(x0, x1, y0, y1, z0, z1)
        if colour == 'module':      # the pegs pass through its four holes
            for ex in (HOLE_INSET, MOD_L - HOLE_INSET):
                for s in (-1, 1): solid -= cyl(1.5, MOD0 + ex, s * (MOD_W/2 - HOLE_INSET), BOARD_Z - 1, UNDER + 1)
        a, b = abs((tray ^ solid).volume()), abs((panel ^ solid).volume())
        ok &= a < 1e-3 and b < 1e-3
        print(f'  {group:8s} {colour:7s} overlap with tray {a:6.3f}  with panel {b:6.3f} mm3')
    boxes = [b[2] for b in part_boxes()]
    for i in range(len(boxes)):                      # the parts must not overlap each other either
        for j in range(i + 1, len(boxes)):
            a, c = boxes[i], boxes[j]
            if all(min(a[k + 3], c[k + 3]) - max(a[k], c[k]) > 1e-6 for k in range(3)):
                ok = False; print('  PARTS OVERLAP:', part_boxes()[i][:2], part_boxes()[j][:2])
    print('OK' if ok else 'PROBLEM')
    if len(sys.argv) > 1:                            # optional: write the layout for the 3D simulator
        json.dump({'L': L, 'W': W, 'H': H, 'UNDER': UNDER, 'PANEL_T': PANEL_T, 'MODX': MODX, 'PADX': PADX,
                   'pads': PADS, 'boxes': part_boxes(), 'wires': wires()}, open(sys.argv[1], 'w'))
