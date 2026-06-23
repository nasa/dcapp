# ADI Texture Utilities

Run these from the repo root:

```bash
python3 samples/adi/utils/generate-adi-texture.py
python3 samples/adi/utils/generate-adi-ufo-texture.py
```

The scripts write editable SVGs and baked PNGs under `assets/`.

## How To Draw

Use normal SVG blocks for backgrounds and custom art:

```python
block(tex, """
<defs>
  <style>
    .mark { stroke: white; stroke-width: 4; }
  </style>
</defs>
""")

rect(tex, 0, 0, tex.width, tex.height / 2, fill="#306fc8")
```

Use `surface_*` for text or marks that should keep the same size when wrapped
onto the ADI sphere:

```python
surface_text(tex, lon=30, lat=45, value="30", size=44, fill="#ffd966")
surface_hline(tex, lon=0, lat=-30, x1=-240, x2=240,
              stroke="#ffffff", width=4)
```

`lon` and `lat` are degrees. The helper widens the object in the flat SVG by
`sec(latitude)`, which cancels the sphere's longitude squeeze.
