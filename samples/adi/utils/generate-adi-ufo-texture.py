#!/usr/bin/env python3
"""Generate the UFO-style ADI sphere texture."""

from __future__ import annotations

import argparse

import adi_texture as adi


def draw_defs(tex: adi.Texture) -> None:
    # This is intentionally just SVG. For custom textures, editing or replacing
    # this block is usually easier than building every gradient/pattern in code.
    adi.block(
        tex,
        """
        <defs>
          <linearGradient id="skyGrad" x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" stop-color="#b48cff"/>
            <stop offset="35%" stop-color="#5b2dd6"/>
            <stop offset="70%" stop-color="#1a0b44"/>
            <stop offset="100%" stop-color="#02010a"/>
          </linearGradient>
          <linearGradient id="gndGrad" x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" stop-color="#1a3b2a"/>
            <stop offset="55%" stop-color="#0b2418"/>
            <stop offset="100%" stop-color="#020a05"/>
          </linearGradient>
          <linearGradient id="vignette" x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" stop-color="#ffffff" stop-opacity="0.18"/>
            <stop offset="50%" stop-color="#000000" stop-opacity="0.42"/>
            <stop offset="100%" stop-color="#ffffff" stop-opacity="0.18"/>
          </linearGradient>
          <linearGradient id="nebulaA" x1="0" y1="0" x2="1" y2="0">
            <stop offset="0%" stop-color="#ff73c8" stop-opacity="0.00"/>
            <stop offset="35%" stop-color="#ff73c8" stop-opacity="0.18"/>
            <stop offset="65%" stop-color="#7cf6ff" stop-opacity="0.14"/>
            <stop offset="100%" stop-color="#7cf6ff" stop-opacity="0.00"/>
          </linearGradient>
          <linearGradient id="nebulaB" x1="0" y1="0" x2="1" y2="0">
            <stop offset="0%" stop-color="#ffe36a" stop-opacity="0.00"/>
            <stop offset="45%" stop-color="#ffe36a" stop-opacity="0.14"/>
            <stop offset="55%" stop-color="#a7ff83" stop-opacity="0.10"/>
            <stop offset="100%" stop-color="#a7ff83" stop-opacity="0.00"/>
          </linearGradient>
          <pattern id="starsDense" width="256" height="256" patternUnits="userSpaceOnUse">
            <circle cx="18" cy="22" r="1.2" fill="#ffffff" opacity="0.55"/>
            <circle cx="40" cy="60" r="1.6" fill="#ffffff" opacity="0.85"/>
            <circle cx="62" cy="190" r="1.1" fill="#ffffff" opacity="0.45"/>
            <circle cx="88" cy="120" r="1.3" fill="#ffffff" opacity="0.60"/>
            <circle cx="90" cy="180" r="1.9" fill="#ffffff" opacity="0.75"/>
            <circle cx="115" cy="35" r="1.1" fill="#ffffff" opacity="0.50"/>
            <circle cx="130" cy="30" r="1.3" fill="#ffffff" opacity="0.65"/>
            <circle cx="150" cy="160" r="1.0" fill="#ffffff" opacity="0.40"/>
            <circle cx="180" cy="90" r="1.2" fill="#ffffff" opacity="0.55"/>
            <circle cx="200" cy="40" r="1.0" fill="#ffffff" opacity="0.38"/>
            <circle cx="210" cy="210" r="1.0" fill="#ffffff" opacity="0.45"/>
            <circle cx="235" cy="20" r="1.4" fill="#ffffff" opacity="0.60"/>
            <circle cx="245" cy="130" r="1.1" fill="#ffffff" opacity="0.50"/>
            <path d="M 170 210 L 172 216 L 178 218 L 172 220 L 170 226 L 168 220 L 162 218 L 168 216 Z" fill="#ffffff" opacity="0.45"/>
            <path d="M 30 140 L 32 146 L 38 148 L 32 150 L 30 156 L 28 150 L 22 148 L 28 146 Z" fill="#ffffff" opacity="0.40"/>
          </pattern>
          <pattern id="rocks" width="220" height="220" patternUnits="userSpaceOnUse">
            <ellipse cx="30" cy="40" rx="18" ry="12" fill="#000000" opacity="0.22"/>
            <ellipse cx="95" cy="70" rx="14" ry="10" fill="#000000" opacity="0.18"/>
            <ellipse cx="160" cy="45" rx="20" ry="14" fill="#000000" opacity="0.20"/>
            <ellipse cx="55" cy="140" rx="22" ry="16" fill="#000000" opacity="0.20"/>
            <ellipse cx="140" cy="145" rx="16" ry="12" fill="#000000" opacity="0.18"/>
            <ellipse cx="190" cy="170" rx="18" ry="13" fill="#000000" opacity="0.19"/>
            <ellipse cx="26" cy="36" rx="6" ry="4" fill="#ffffff" opacity="0.06"/>
            <ellipse cx="90" cy="66" rx="5" ry="3" fill="#ffffff" opacity="0.05"/>
            <ellipse cx="154" cy="41" rx="6" ry="4" fill="#ffffff" opacity="0.05"/>
            <ellipse cx="50" cy="136" rx="7" ry="5" fill="#ffffff" opacity="0.05"/>
            <ellipse cx="135" cy="141" rx="5" ry="3" fill="#ffffff" opacity="0.05"/>
            <ellipse cx="184" cy="166" rx="6" ry="4" fill="#ffffff" opacity="0.05"/>
            <circle cx="115" cy="120" r="2" fill="#000000" opacity="0.18"/>
            <circle cx="205" cy="25" r="2" fill="#000000" opacity="0.14"/>
            <circle cx="15" cy="205" r="2" fill="#000000" opacity="0.14"/>
          </pattern>
          <radialGradient id="beamGlow" cx="50%" cy="0%" r="90%">
            <stop offset="0%" stop-color="#7cf6ff" stop-opacity="0.34"/>
            <stop offset="70%" stop-color="#7cf6ff" stop-opacity="0.10"/>
            <stop offset="100%" stop-color="#7cf6ff" stop-opacity="0.00"/>
          </radialGradient>
          <style>
            .grid { stroke: #ffffff; stroke-width: 2; opacity: 0.18; }
            .horizon { stroke: #ff7af6; stroke-width: 6; opacity: 0.92; }
            .pitchBar { stroke: #7cf6ff; stroke-width: 4; opacity: 0.80; }
            .contour { stroke: #3cff9e; stroke-width: 2; opacity: 0.28; fill: none; }
            .ufoLine { stroke: #ffe36a; stroke-width: 5; opacity: 0.95; fill: none; }
            .ufoFill { fill: #3a2b6f; opacity: 0.95; }
            .ufoGlass { fill: #7cf6ff; opacity: 0.55; }
            .ufoLight { fill: #ffe36a; opacity: 0.88; }
            .rollLbl { fill: #ffd966; font-family: DejaVu Sans, Liberation Sans, Arial, sans-serif; font-size: 44px; font-weight: 700; opacity: 0.88; }
          </style>
        </defs>
        """,
    )


def draw_background(tex: adi.Texture) -> None:
    half = tex.height / 2

    # Large fills and patterns stay in flat texture space. They are background
    # material, so normal equirectangular stretching is acceptable.
    adi.rect(tex, 0, 0, tex.width, half, fill="url(#skyGrad)")
    adi.rect(tex, 0, half, tex.width, half, fill="url(#gndGrad)")
    adi.rect(tex, 0, 180, tex.width, 220, fill="url(#nebulaA)")
    adi.rect(tex, 0, 520, tex.width, 240, fill="url(#nebulaB)")
    adi.rect(tex, 0, 0, tex.width, half, fill="url(#starsDense)")
    adi.rect(tex, 0, half, tex.width, half, fill="url(#rocks)", opacity=0.95)
    adi.rect(tex, 0, 0, tex.width, tex.height, fill="url(#vignette)")
    adi.line(tex, 0, half, tex.width, half, class_name="horizon")


def draw_marks(tex: adi.Texture) -> None:
    # Keep pitch bars in latitude space, just like the number labels. The
    # earlier version used old flat-SVG y coordinates, which made the bars look
    # close to the label rows but not exactly on them.
    for lat, half_width in [
        (45, 648),
        (30, 598),
        (15, 548),
        (-15, 548),
        (-30, 598),
        (-45, 648),
    ]:
        adi.surface_hline(tex, 0, lat, -half_width, half_width, class_name="pitchBar")

    # Decorative contour arcs remain flat SVG. They are broad background shapes,
    # not precise labels, so keeping the original art is more useful.
    adi.block(
        tex,
        """
        <g>
          <ellipse class="contour" cx="2048" cy="1180" rx="1500" ry="180"/>
          <ellipse class="contour" cx="2048" cy="1420" rx="1050" ry="150"/>
          <ellipse class="contour" cx="2048" cy="1660" rx="650" ry="115"/>
          <ellipse class="contour" cx="2048" cy="1900" rx="260" ry="70"/>
        </g>
        """,
    )

    adi.add(tex, "  <g>")
    for lon in range(-150, 180, 30):
        x = (lon + 180) / 360 * tex.width
        adi.add(tex, f'    <line class="grid" x1="{x:.3f}" y1="0" x2="{x:.3f}" y2="{tex.height}"/>')
    adi.add(tex, "  </g>")

    # Number labels are the main reason for this utility. They are deliberately
    # widened in the flat map so they appear consistent once projected.
    adi.surface_label_grid(
        tex,
        [-150, -120, -90, -60, -30, 0, 30, 60, 90, 120, 150],
        [75, 45, 15, -15, -45, -75],
        44,
        "#ffd966",
        0.88,
        class_name="rollLbl",
    )


def draw_ufo(tex: adi.Texture) -> None:
    # Inject arbitrary SVG art this way. You can replace this block with an
    # imported SVG fragment, hand-authored paths, or anything Inkscape can
    # rasterize.
    adi.block(
        tex,
        """
        <g transform="translate(780,230)">
          <path d="M 240 140 L 110 760 L 370 760 Z" fill="url(#beamGlow)" opacity="0.7"/>
          <ellipse cx="240" cy="120" rx="240" ry="70" class="ufoFill"/>
          <path d="M 25 120 Q 240 15 455 120" class="ufoLine"/>
          <ellipse cx="240" cy="75" rx="95" ry="60" class="ufoGlass"/>
          <circle cx="110" cy="135" r="15" class="ufoLight"/>
          <circle cx="170" cy="148" r="15" class="ufoLight"/>
          <circle cx="240" cy="154" r="15" class="ufoLight"/>
          <circle cx="310" cy="148" r="15" class="ufoLight"/>
          <circle cx="370" cy="135" r="15" class="ufoLight"/>
          <line x1="240" y1="8" x2="240" y2="42" class="ufoLine"/>
          <circle cx="240" cy="7" r="9" fill="#ffe36a" opacity="0.9"/>
        </g>
        """,
    )


def build_texture(width: int, height: int) -> adi.Texture:
    tex = adi.Texture(width, height)
    adi.begin(tex, "samples/adi/utils/generate-adi-ufo-texture.py", "UFO texture with latitude-corrected labels and pitch bars.")
    draw_defs(tex)
    draw_background(tex)
    draw_marks(tex)
    draw_ufo(tex)
    adi.end(tex)
    return tex


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate the UFO-style ADI equirectangular sphere texture.")
    adi.add_output_args(parser, adi.repo_path("assets", "adi-ufo.svg"), adi.repo_path("assets", "adi-ufo.png"))
    args = parser.parse_args()
    adi.write_outputs(build_texture(args.width, args.height), args)


if __name__ == "__main__":
    main()
