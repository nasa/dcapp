# PixelStream

`PixelStream` draws live image streams inside a dcapp display. The current XML
surface supports MJPEG streams and shared-memory streams.

Use `PixelStream` when the content is already an image/video feed, such as a
camera, renderer output, or shared-memory frame source. Use normal primitives or
`DrawFunction` when the display should draw the content itself from values.

Pixel streams participate in layout like other drawables, and they can also be
mouse targets. That makes them useful for camera panes with overlays,
click-to-select regions, or video-backed controls.

## MJPEG XML Shape

```xml
<PixelStream Type="#_pixelstream_mjpeg_"
             URL="http://localhost:8090/stream"
             Timeout="5"
             X="80" Y="30"
             Width="640" Height="480"/>
```

## Attributes

| Attribute | Meaning |
|-----------|---------|
| `Type` | `#_pixelstream_mjpeg_` or `#_pixelstream_shmem_` |
| `URL` | MJPEG HTTP endpoint |
| `Protocol` | Optional protocol hint |
| `Timeout` | Connection/read timeout |
| `TestPattern` | Draw a fallback/test pattern |
| `X`, `Y`, `Width`, `Height` | Standard drawable placement |

`PixelStream` is also a mouse target, so it can contain the same mouse event
children as other drawable targets.

## Sample

Start the test server, then launch the display:

```bash
python3 samples/pixelstream-mjpeg/server.py
./bin/dcapp.sh samples/pixelstream-mjpeg/pixelstream-mjpeg.xml
```

See also [integration.md](integration.md) for the cross-protocol overview.
