# External integrations

dcapp supports three live-data paths:

| Integration | Data source | Reference |
|-------------|------------|---------|
| TrickIO | Trick Variable Server simulation variables | [trick.md](trick.md) |
| EdgeIO | Edge RCS command/value exchange | [edge.md](edge.md) |
| PixelStream | Video/image stream display | [pixelstream.md](pixelstream.md) |

Declare `TrickIO` and `EdgeIO` directly under `DCAPP`, usually beside the
`Variable` declarations. Their mapping children contain a dcapp variable name,
not a value expression, so do not prefix the name with `@`. `PixelStream` is a
drawable and belongs inside the `Window` render tree.
