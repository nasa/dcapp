#!/usr/bin/env python3
"""Generate the clean ADI sphere texture."""

from __future__ import annotations

import argparse
import random

import adi_texture as adi


def draw_defs(tex: adi.Texture) -> None:
    # Definitions are regular SVG. Edit these gradients/classes the same way
    # you would edit a hand-authored SVG file.
    adi.block(
        tex,
        """
        <defs>
          <linearGradient id="sky" x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" stop-color="#65b8ff"/>
            <stop offset="44%" stop-color="#1d6fc8"/>
            <stop offset="100%" stop-color="#0d2f66"/>
          </linearGradient>
          <linearGradient id="ground" x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" stop-color="#8a612e"/>
            <stop offset="56%" stop-color="#4c351e"/>
            <stop offset="100%" stop-color="#21170e"/>
          </linearGradient>
          <linearGradient id="haze" x1="0" y1="0" x2="0" y2="1">
            <stop offset="0%" stop-color="#ffffff" stop-opacity="0.16"/>
            <stop offset="48%" stop-color="#ffffff" stop-opacity="0.00"/>
            <stop offset="52%" stop-color="#000000" stop-opacity="0.00"/>
            <stop offset="100%" stop-color="#000000" stop-opacity="0.22"/>
          </linearGradient>
          <style>
            text { font-family: DejaVu Sans, Liberation Sans, Arial, sans-serif; }
            .ladder { stroke: #f5f8ff; }
            .minor { stroke: #c8ddff; }
            .grid { stroke: #ffffff; stroke-width: 2; opacity: 0.14; }
          </style>
        </defs>
        """,
    )


def draw_background(tex: adi.Texture) -> None:
    half = tex.height / 2

    # Background fills are normal flat SVG. They are not surface-corrected
    # because broad color fields are supposed to stretch over the sphere.
    adi.rect(tex, 0, 0, tex.width, half, fill="url(#sky)")
    adi.rect(tex, 0, half, tex.width, half, fill="url(#ground)")
    adi.rect(tex, 0, 0, tex.width, tex.height, fill="url(#haze)")

    # Small decorative dots are surface-corrected so they do not turn into
    # skinny streaks as they approach the top or bottom of the ball.
    rng = random.Random(19)
    for _ in range(180):
        lat = rng.uniform(8.0, 82.0)
        lon = rng.uniform(-180.0, 180.0)
        radius = rng.uniform(1.4, 3.8)
        opacity = rng.uniform(0.22, 0.58)
        adi.surface_ellipse(tex, lon, lat, radius, radius, "#ffffff", opacity)

    for _ in range(220):
        lat = rng.uniform(-82.0, -6.0)
        lon = rng.uniform(-180.0, 180.0)
        radius = rng.uniform(3.0, 9.0)
        opacity = rng.uniform(0.08, 0.19)
        adi.surface_ellipse(tex, lon, lat, radius * 1.4, radius * 0.55, "#120d08", opacity)

    for lon in range(-150, 181, 30):
        x = adi.lon_x(tex, lon)
        adi.line(tex, x, 0, x, tex.height, class_name="grid")

    for lat in range(-60, 61, 30):
        y = adi.lat_y(tex, lat)
        adi.line(tex, 0, y, tex.width, y, class_name="grid")


def draw_pitch_ladder(tex: adi.Texture) -> None:
    adi.add(tex, '  <g id="surface-corrected-adi-marks">')

    # The horizon lies at latitude 0, so there is no horizontal sphere squeeze
    # to correct here.
    y = adi.lat_y(tex, 0)
    adi.line(tex, 0, y, tex.width, y, stroke="#fff4b0", stroke_width=10, opacity=0.96)
    adi.line(tex, 0, y, tex.width, y, stroke="#111111", stroke_width=2, opacity=0.45)

    # Pitch ladder bars and labels are placed by lon/lat. The x distances below
    # are local pixels around lon=0, then adi.surface_* handles the projection
    # correction for the requested latitude.
    for pitch in [-75, -60, -45, -30, -20, -10, 10, 20, 30, 45, 60, 75]:
        major = abs(pitch) in (30, 60)
        extreme = abs(pitch) == 75
        width = 6 if major else 4
        opacity = 0.62 if extreme else 0.90
        line_class = "ladder" if major else "minor"

        adi.surface_hline(tex, 0, pitch, -380 if major else -300, -62, "#f5f8ff", width, opacity, line_class)
        adi.surface_hline(tex, 0, pitch, 62, 380 if major else 300, "#f5f8ff", width, opacity, line_class)

        if abs(pitch) % 20 == 0 or major or extreme:
            color = "#f5f8ff" if pitch > 0 else "#ffe0a6"
            size = 40 if extreme else 46
            adi.surface_text(tex, 0, pitch, f"{pitch:+d}", size, color, x=-455, y=1, opacity=opacity)
            adi.surface_text(tex, 0, pitch, f"{pitch:+d}", size, color, x=455, y=1, opacity=opacity)

    for offset in [-300, -150, 0, 150, 300]:
        adi.surface_vline(tex, 0, 0, offset, -26, 26, "#fff4b0", 5, 0.78)

    adi.surface_text(tex, 0, 0, "ADI", 42, "#fff4b0", y=-62, opacity=0.90)

    # These high-latitude labels intentionally look wide in the flat SVG. On
    # the sphere they compress back toward the intended visual size.
    for lat, label in [(72, "NORTH"), (-72, "SOUTH")]:
        for lon in [-120, 0, 120]:
            adi.surface_text(tex, lon, lat, label, 38, "#fff4b0", opacity=0.54, weight=800)

    adi.add(tex, "  </g>")


def build_texture(width: int, height: int) -> adi.Texture:
    tex = adi.Texture(width, height)
    adi.begin(tex, "samples/adi/utils/generate-adi-texture.py", "Surface marks are horizontally pre-scaled by sec(latitude).")
    draw_defs(tex)
    draw_background(tex)
    draw_pitch_ladder(tex)
    adi.end(tex)
    return tex


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate the clean ADI equirectangular sphere texture.")
    adi.add_output_args(parser, adi.repo_path("assets", "adi.svg"), adi.repo_path("assets", "adi.png"))
    args = parser.parse_args()
    adi.write_outputs(build_texture(args.width, args.height), args)


if __name__ == "__main__":
    main()
