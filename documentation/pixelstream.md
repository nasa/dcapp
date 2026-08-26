# PixelStream

`PixelStream` is a drawable for MJPEG and shared-memory image streams. It uses
the normal placement attributes and can contain mouse-event children.

## MJPEG

```xml
<PixelStream Type="#_pixelstream_mjpeg_"
             URL="http://localhost:8090/stream"
             Timeout="5"
             X="80" Y="30"
             Width="640" Height="480"/>
```

The sample server requires Pillow:

```bash
python3 -m pip install pillow
```

Shared-memory streams are available on Linux and macOS. `File` is the backing
file used to derive the System V shared-memory key and read RGBA frames;
`SharedMemoryKey` remains available as an alias.

```xml
<PixelStream Type="#_pixelstream_shmem_"
             File="/tmp/camera.rgba"
             Width="640" Height="480"/>
```

## Attributes

| Attribute | Meaning |
|-----------|---------|
| `Type` | Required: `#_pixelstream_mjpeg_` or `#_pixelstream_shmem_` |
| `URL` | MJPEG HTTP endpoint |
| `File`, `SharedMemoryKey` | Shared-memory backing-file path |
| `Timeout` | MJPEG connection/read timeout in seconds; defaults to `5` |
| `TestPattern` | Fallback image path; defaults to `assets/testpattern.png` |
| `X`, `Y`, `Width`, `Height` | Standard drawable placement |

## Running the MJPEG sample

Start the test server, then launch the display:

```bash
python3 samples/pixelstream-mjpeg/server.py
./bin/dcapp.sh samples/pixelstream-mjpeg/pixelstream-mjpeg.xml
```
