#!/usr/bin/env python3
"""
MJPEG test server for dcapp PixelStream sample.

Generates animated JPEG frames and serves them as a multipart/x-mixed-replace
MJPEG stream on http://localhost:8090/stream

Requirements: pip install pillow

Usage: python3 server.py [--port PORT] [--fps FPS] [--width WIDTH] [--height HEIGHT]
"""

import argparse
import io
import math
import time
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    print("Error: Pillow is required. Install with: pip install pillow")
    raise SystemExit(1)

BOUNDARY = b"--frame"


class MjpegStreamHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        if self.path == "/stream":
            self._handle_stream()
        elif self.path == "/":
            self._handle_index()
        else:
            self.send_error(404)

    def _handle_index(self):
        html = (
            "<html><body style='background:#111;color:#eee;font-family:monospace'>"
            "<h2>MJPEG Test Server</h2>"
            "<p>Stream URL: <a href='/stream'>/stream</a></p>"
            "<img src='/stream' style='border:1px solid #444'/>"
            "</body></html>"
        )
        self.send_response(200)
        self.send_header("Content-Type", "text/html")
        self.end_headers()
        self.wfile.write(html.encode())

    def _handle_stream(self):
        self.send_response(200)
        self.send_header("Content-Type", f"multipart/x-mixed-replace; boundary=frame")
        self.send_header("Cache-Control", "no-cache")
        self.send_header("Connection", "keep-alive")
        self.end_headers()

        width = self.server.frame_width
        height = self.server.frame_height
        fps = self.server.fps
        frame_num = 0
        interval = 1.0 / fps

        try:
            while True:
                t0 = time.monotonic()
                jpeg_bytes = generate_frame(width, height, frame_num)
                self.wfile.write(BOUNDARY + b"\r\n")
                self.wfile.write(b"Content-Type: image/jpeg\r\n")
                self.wfile.write(f"Content-Length: {len(jpeg_bytes)}\r\n".encode())
                self.wfile.write(b"\r\n")
                self.wfile.write(jpeg_bytes)
                self.wfile.write(b"\r\n")
                self.wfile.flush()
                frame_num += 1
                elapsed = time.monotonic() - t0
                if elapsed < interval:
                    time.sleep(interval - elapsed)
        except (BrokenPipeError, ConnectionResetError):
            pass

    def log_message(self, format, *args):
        # quieter logging
        if "/stream" not in str(args):
            BaseHTTPRequestHandler.log_message(self, format, *args)


def generate_frame(width, height, frame_num):
    """Generate an animated test frame as JPEG bytes."""
    img = Image.new("RGB", (width, height), (20, 20, 30))
    draw = ImageDraw.Draw(img)

    t = frame_num * 0.05

    # grid
    for x in range(0, width, 40):
        draw.line([(x, 0), (x, height)], fill=(40, 40, 50))
    for y in range(0, height, 40):
        draw.line([(0, y), (width, y)], fill=(40, 40, 50))

    # bouncing circle
    cx = int(width / 2 + math.sin(t) * width * 0.3)
    cy = int(height / 2 + math.cos(t * 0.7) * height * 0.25)
    r = 40
    draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(50, 130, 255))
    draw.ellipse([cx - r + 4, cy - r + 4, cx + r - 4, cy + r - 4], fill=(80, 160, 255))

    # orbiting dots
    for i in range(6):
        angle = t * (1.0 + i * 0.2) + i * math.pi / 3
        ox = int(width / 2 + math.cos(angle) * 120)
        oy = int(height / 2 + math.sin(angle) * 90)
        colors = [
            (255, 100, 100), (100, 255, 100), (100, 100, 255),
            (255, 255, 100), (255, 100, 255), (100, 255, 255),
        ]
        dr = 8
        draw.ellipse([ox - dr, oy - dr, ox + dr, oy + dr], fill=colors[i])

    # scanning line
    scan_y = int((frame_num * 3) % height)
    draw.line([(0, scan_y), (width, scan_y)], fill=(60, 200, 60, 128), width=2)

    # frame counter text
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 18)
        small_font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14)
    except (OSError, IOError):
        font = ImageFont.load_default()
        small_font = font

    draw.text((10, 10), f"MJPEG Test Stream", fill=(200, 200, 220), font=font)
    draw.text((10, 35), f"Frame: {frame_num}", fill=(150, 150, 170), font=small_font)
    draw.text((10, 55), f"{width}x{height}", fill=(150, 150, 170), font=small_font)

    # border
    draw.rectangle([0, 0, width - 1, height - 1], outline=(80, 80, 100))

    buf = io.BytesIO()
    img.save(buf, format="JPEG", quality=80)
    return buf.getvalue()


def main():
    parser = argparse.ArgumentParser(description="MJPEG test server for dcapp")
    parser.add_argument("--port", type=int, default=8090, help="Port (default: 8090)")
    parser.add_argument("--fps", type=int, default=15, help="Frames per second (default: 15)")
    parser.add_argument("--width", type=int, default=640, help="Frame width (default: 640)")
    parser.add_argument("--height", type=int, default=480, help="Frame height (default: 480)")
    args = parser.parse_args()

    server = HTTPServer(("0.0.0.0", args.port), MjpegStreamHandler)
    server.frame_width = args.width
    server.frame_height = args.height
    server.fps = args.fps

    print(f"MJPEG server running on http://localhost:{args.port}/stream")
    print(f"  Resolution: {args.width}x{args.height} @ {args.fps} fps")
    print(f"  Open http://localhost:{args.port}/ in a browser to preview")
    print(f"  Press Ctrl+C to stop")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down...")
        server.shutdown()


if __name__ == "__main__":
    main()
