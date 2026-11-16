"""PalabraOS enclosure generator.  Run:  python3 make_enclosure.py
Needs: pip install manifold3d trimesh numpy
Writes back_tray.stl and front_panel.stl (millimetres, already oriented for printing).
Change the numbers in the PARAMETERS block and run again to resize anything."""
import numpy as np, trimesh
from manifold3d import Manifold, CrossSection

# ---------------- PARAMETERS (mm) ----------------
MOD_L, MOD_W, PCB_T = 72.0, 30.5, 3.0      # display module board
PANEL_L, PANEL_T    = 59.5, 1.0            # white panel on the board (centred along the board)
HOLE_INSET          = 2.75                 # hole centre to each board edge   <-- measure and adjust
PEG_D               = 2.6                  # peg for the 3 mm holes (0.2 mm slack each way)
SOCKET_T            = 2.0                  # cable socket height on the back of the board
XIAO_L, XIAO_W, XIAO_T = 21.0, 17.8, 1.2   # XIAO ESP32-S3 board
BAT_L, BAT_W, BAT_T = 35.0, 30.0, 5.0      # 503035 LiPo
SW, SW_BODY         = 6.0, 3.5             # tactile switch body 6 x 6 x 3.5
PITCH               = 8.0                  # button spacing
WALL, FLOOR, PLATE  = 1.6, 1.5, 1.5
GAP, TAB, PADZONE   = 0.4, 1.5, 24.0
R                   = 3.0                  # outer corner radius
H                   = 13.0                 # total thickness

# ---------------- derived ----------------
IN_L = TAB + GAP + MOD_L + GAP + PADZONE
IN_W = MOD_W + 2 * GAP
L, W = IN_L + 2 * WALL, IN_W + 2 * WALL
X0 = -IN_L / 2                       # inside face of the left wall
MOD0 = X0 + TAB + GAP                # left end of the module
MODX = MOD0 + MOD_L / 2
PADX = MOD0 + MOD_L + GAP + PADZONE / 2
UNDER = H - PLATE                    # underside of the front panel = top of the walls
BOARD_Z = UNDER - PCB_T              # back face of the module board
PADS = {'U': (0, PITCH), 'D': (0, -PITCH), 'L': (-PITCH, 0), 'R': (PITCH, 0), 'C': (0, 0)}
XB = FLOOR + 1.0                     # underside of the XIAO board (raised for the battery pads)
BX0 = MOD0 + 22 + 6 + 0.6            # battery starts just past the cable socket
BX1 = BX0 + BAT_L + 0.5

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
    # USB-C opening in the left end wall
    cut.append(box(-L/2 - 1, X0 + 0.01, -5.0, 5.0, XB + XIAO_T - 0.3, XB + XIAO_T + 3.6))
    # XIAO cradle: a ledge under each long edge, a low fence outside it, a stop at the inner end
    xe = X0 + 0.2 + XIAO_L
    for s in (-1, 1):
        add.append(box(X0, xe + 0.2, s * (XIAO_W/2 - 0.8), s * (XIAO_W/2 + 0.2), FLOOR, XB))
        add.append(box(X0, xe + 0.2, s * (XIAO_W/2 + 0.2), s * (XIAO_W/2 + 1.4), FLOOR, XB + XIAO_T + 0.4))
    add.append(box(xe - 0.8, xe + 0.2, -6, 6, FLOOR, XB))
    add.append(box(xe + 0.2, xe + 1.4, -6, 6, FLOOR, XB + XIAO_T + 0.4))
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

# stand-ins for the real parts, used only to check that nothing collides
def components():
    c = {}
    board = box(MOD0, MOD0 + MOD_L, -MOD_W/2, MOD_W/2, BOARD_Z, UNDER)
    for ex in (HOLE_INSET, MOD_L - HOLE_INSET):
        for s in (-1, 1): board -= cyl(1.5, MOD0 + ex, s * (MOD_W/2 - HOLE_INSET), BOARD_Z - 1, UNDER + 1)
    c['module board'] = board
    c['display panel'] = box(MODX - PANEL_L/2, MODX + PANEL_L/2, -MOD_W/2, MOD_W/2, UNDER, UNDER + PANEL_T)
    c['cable socket'] = box(MOD0 + 22, MOD0 + 28, -7.5, 7.5, BOARD_Z - SOCKET_T, BOARD_Z)
    c['XIAO'] = (box(X0 + 0.2, X0 + 0.2 + XIAO_L, -XIAO_W/2, XIAO_W/2, XB, XB + XIAO_T)
                 + box(X0 - 1.2, X0 + 6.2, -4.5, 4.5, XB + XIAO_T, XB + XIAO_T + 3.2)
                 + box(X0 + 8, X0 + 19, -6, 6, XB + XIAO_T, XB + XIAO_T + 2.3))
    c['battery'] = box(BX0 + 0.25, BX0 + 0.25 + BAT_L, -BAT_W/2, BAT_W/2, FLOOR, FLOOR + BAT_T)
    c['switches'] = union([box(PADX + dx - 3, PADX + dx + 3, dy - 3, dy + 3, UNDER - SW_BODY, UNDER) + cyl(1.75, PADX + dx, dy, UNDER, UNDER + 2.5)
                           for dx, dy in PADS.values()])
    return c

def to_trimesh(m):
    mesh = m.to_mesh()
    return trimesh.Trimesh(np.asarray(mesh.vert_properties)[:, :3], np.asarray(mesh.tri_verts), process=True)

if __name__ == '__main__':
    tray, panel = back_tray(), front_panel()
    print(f'outside size: {L:.1f} x {W:.1f} x {H:.1f} mm')
    ok = True
    for name, part in (('back_tray', tray), ('front_panel', panel)):
        out = part if name == 'back_tray' else part.rotate([180, 0, 0])      # panel prints face down
        tm = to_trimesh(out); tm.apply_translation([0, 0, -tm.bounds[0][2]])
        tm.export(name + '.stl')
        size = tm.bounds[1] - tm.bounds[0]
        pieces = len(part.decompose())
        print(f'{name}.stl: {size[0]:.1f} x {size[1]:.1f} x {size[2]:.1f} mm, {tm.volume/1000:.1f} cm3, watertight={tm.is_watertight}, pieces={pieces}')
        ok &= tm.is_watertight and pieces == 1
    print(f'tray vs panel overlap: {(tray ^ panel).volume():.3f} mm3')
    for cname, comp in components().items():
        print(f'  {cname:14s} overlap with tray {(tray ^ comp).volume():7.3f}  with panel {(panel ^ comp).volume():7.3f} mm3')
    print('OK' if ok else 'PROBLEM')
