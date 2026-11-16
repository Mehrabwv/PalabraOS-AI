# PalabraOS enclosures

Two 3D-printable cases for the wired build (no circuit board). Both are 101.5 × 34.5 mm.

| | Default (`enclosure/`) | Slim (`enclosure/slim/`) |
|---|---|---|
| Thickness | 13 mm | 10.5 mm |
| Files | `back_tray.stl`, `front_panel.stl` | `back_tray_slim.stl`, `front_panel_slim.stl` |
| Display module | solder holes away from the buttons | turned end for end, holes next to the buttons |
| XIAO | lengthwise, USB-C out of the short end | crosswise, USB-C out of the top long edge |
| Display wiring | 8-wire cable from the socket | 8 short wires soldered to the holes |
| Firmware | `EPD_ROTATION 1` | `EPD_ROTATION 3` |

Neither case has been printed or test-fitted yet. Hole centres are assumed
2.75 mm from each edge of the display board.

## Printing

- Print both parts as they come: tray floor-down, front panel face-down. No supports.
- 0.2 mm layers or finer. PETG is kinder to the snap clips than PLA.

## Assembly

1. Push the display module onto the four pegs of the front panel.
2. Drop the five switches into their collars.
3. Put the XIAO and battery in the tray and solder the wires.
4. Hook the two tabs at the left end into the tray, then press the button end down
   until the two clips click. To open, push the clips in through the two small windows.

Slim only: put tape over the back of the display board where the battery touches it.

## Changing a dimension

Each folder has a generator script. Edit the numbers in its PARAMETERS block and run it:

```bash
pip install manifold3d trimesh numpy
```

```bash
python3 make_enclosure_slim.py
```

It rewrites the STL files and prints a fit check (watertight, no part collides with another).
