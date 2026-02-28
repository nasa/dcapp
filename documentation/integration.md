# dcapp External Integration

A guide to connecting dcapp displays with external data sources and streaming services.

---

## Overview

dcapp supports several integration protocols for exchanging data with external systems:

- **TrickIO** -- Connects to the Trick Variable Server for real-time simulation data
- **EdgeIO** -- Connects to the Edge RCS server for command-based data exchange
- **PixelStream** -- Embeds live video streams (MJPEG) into a display

Integration elements are declared at the top level of your dcapp file, alongside Variable declarations and before the `<Window>` element.

---

## TrickIO

TrickIO connects dcapp to a running [Trick](https://github.com/nasa/trick) simulation via its Variable Server protocol. Variables are mapped by name between the Trick simulation and dcapp, with optional unit conversion.

### Element Structure

```xml
<TrickIO Host="localhost" Port="7000" DataRate="0.1">
    <TrickFrom>
        <TrickVariable Name="dyn.cannon.pos[0]" Units="ft">posX_ft</TrickVariable>
    </TrickFrom>
    <TrickTo>
        <TrickVariable Name="dyn.cannon.vel[0]">velCommand</TrickVariable>
    </TrickTo>
</TrickIO>
```

### TrickIO Attributes

| Attribute | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `Host` | string | Yes | -- | Hostname or IP of the Trick simulation |
| `Port` | integer | Yes | -- | Port number of the Trick Variable Server |
| `DataRate` | double | No | `0.1` | Seconds between data updates (0.1 = 10Hz) |
| `ConnectedVariable` | string | No | -- | Optional dcapp variable set to reflect connection status |

Both `Host` and `Port` are required. If either is missing, dcapp will log an error.

### Direction Containers

- **`<TrickFrom>`** -- Variables received from the Trick simulation into dcapp. The Trick Variable Server pushes updated values at the specified DataRate.
- **`<TrickTo>`** -- Variables sent from dcapp to the Trick simulation. When the dcapp variable changes, the new value is written to the Trick simulation.

### TrickVariable Attributes

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Name` | string | Yes | The Trick variable path (e.g., `"dyn.cannon.pos[0]"`) |
| `Units` | string | No | Unit conversion string (e.g., `"ft"`, `"ft/s"`, `"deg"`) |

**Content:** The dcapp variable name that this Trick variable maps to.

When `Units` is specified on a `<TrickFrom>` variable, Trick performs the unit conversion server-side before sending the value. This allows you to receive data in your preferred unit system without manual conversion.

### Multiple Connections

You can declare multiple `<TrickIO>` elements to connect to different Trick servers or to use different data rates for different variable groups. This is useful when some variables need high-frequency updates (e.g., position data at 10Hz) while others can update less often (e.g., status indicators at 1Hz).

### Example: Cannonball Trajectory

This example from `samples/trick/trick.xml` demonstrates two TrickIO connections: a fast 10Hz metric connection and a slower 1Hz imperial connection with unit conversion.

```xml
<DCAPP>
    <!-- Variables for metric data (fast update) -->
    <Variable Type="#_variable_double_" InitialValue="0.0">posX</Variable>
    <Variable Type="#_variable_double_" InitialValue="0.0">posY</Variable>
    <Variable Type="#_variable_double_" InitialValue="0.0">velX</Variable>
    <Variable Type="#_variable_double_" InitialValue="0.0">velY</Variable>
    <Variable Type="#_variable_double_" InitialValue="0.0">simTime</Variable>
    <Variable Type="#_variable_integer_" InitialValue="0">impact</Variable>

    <!-- Variables for imperial data (slower update) -->
    <Variable Type="#_variable_double_" InitialValue="0.0">posX_ft</Variable>
    <Variable Type="#_variable_double_" InitialValue="0.0">posY_ft</Variable>
    <Variable Type="#_variable_double_" InitialValue="0.0">velX_fps</Variable>
    <Variable Type="#_variable_double_" InitialValue="0.0">velY_fps</Variable>

    <!-- Fast connection: 10Hz, metric (no unit conversion) -->
    <TrickIO Host="localhost" Port="7000" DataRate="0.1">
        <TrickFrom>
            <TrickVariable Name="dyn.cannon.pos[0]">posX</TrickVariable>
            <TrickVariable Name="dyn.cannon.pos[1]">posY</TrickVariable>
            <TrickVariable Name="dyn.cannon.vel[0]">velX</TrickVariable>
            <TrickVariable Name="dyn.cannon.vel[1]">velY</TrickVariable>
            <TrickVariable Name="dyn.cannon.time">simTime</TrickVariable>
            <TrickVariable Name="dyn.cannon.impact">impact</TrickVariable>
        </TrickFrom>
    </TrickIO>

    <!-- Slow connection: 1Hz, imperial (with unit conversion) -->
    <TrickIO Host="localhost" Port="7000" DataRate="1.0">
        <TrickFrom>
            <TrickVariable Name="dyn.cannon.pos[0]" Units="ft">posX_ft</TrickVariable>
            <TrickVariable Name="dyn.cannon.pos[1]" Units="ft">posY_ft</TrickVariable>
            <TrickVariable Name="dyn.cannon.vel[0]" Units="ft/s">velX_fps</TrickVariable>
            <TrickVariable Name="dyn.cannon.vel[1]" Units="ft/s">velY_fps</TrickVariable>
        </TrickFrom>
    </TrickIO>

    <Window Title="Cannonball Trajectory" Width="900" Height="700">
        <!-- Display content using @posX, @posY, etc. -->
    </Window>
</DCAPP>
```

To run this sample, start the Trick simulation first:

```bash
cd samples/trick/sim && ./S_main_*.exe RUN_test/input.py
```

Then launch the dcapp display:

```bash
dcapp samples/trick/trick.xml
```

---

## EdgeIO

EdgeIO connects dcapp to an Edge RCS (Remote Command Server) for command-based data exchange. The structure is similar to TrickIO, but variables are identified by command strings rather than Trick variable paths.

### Element Structure

```xml
<EdgeIO Host="localhost" Port="5451" DataRate="1.0">
    <EdgeFrom>
        <EdgeVariable Command="GET_TEMPERATURE">temperature</EdgeVariable>
    </EdgeFrom>
    <EdgeTo>
        <EdgeVariable Command="SET_THROTTLE">throttleCommand</EdgeVariable>
    </EdgeTo>
</EdgeIO>
```

### EdgeIO Attributes

| Attribute | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `Host` | string | No | `"localhost"` | Hostname or IP of the Edge server |
| `Port` | integer | No | `5451` | Port number of the Edge server |
| `DataRate` | double | No | `1.0` | Seconds between data updates |
| `ConnectedVariable` | string | No | -- | Optional dcapp variable set to reflect connection status |

Unlike TrickIO, all EdgeIO attributes have defaults, so a minimal declaration is possible:

```xml
<EdgeIO>
    <!-- Uses localhost:5451 at 1Hz -->
</EdgeIO>
```

### Direction Containers

- **`<EdgeFrom>`** -- Variables received from the Edge server into dcapp.
- **`<EdgeTo>`** -- Variables sent from dcapp to the Edge server. Values are transmitted when the dcapp variable changes.

### EdgeVariable Attributes

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Command` | string | Yes | The Edge command string |

**Content:** The dcapp variable name that this Edge variable maps to.

The `Command` attribute is required. If it is missing, dcapp will log an error.

### Example

```xml
<DCAPP>
    <Variable Type="#_variable_double_" InitialValue="0.0">sensorValue</Variable>
    <Variable Type="#_variable_double_" InitialValue="0.0">controlOutput</Variable>
    <Variable Type="#_variable_boolean_" InitialValue="false">isConnected</Variable>

    <EdgeIO Host="192.168.1.100" Port="5451" DataRate="0.5" ConnectedVariable="isConnected">
        <EdgeFrom>
            <EdgeVariable Command="READ_SENSOR_1">sensorValue</EdgeVariable>
        </EdgeFrom>
        <EdgeTo>
            <EdgeVariable Command="WRITE_CONTROL_1">controlOutput</EdgeVariable>
        </EdgeTo>
    </EdgeIO>

    <Window Title="Edge Control Panel" Width="600" Height="400">
        <!-- Display content -->
    </Window>
</DCAPP>
```

---

## PixelStream

PixelStream embeds a live video stream into a dcapp display. Currently, the MJPEG streaming mode is supported, which reads frames from an HTTP endpoint serving a multipart JPEG stream.

### Element Structure

```xml
<PixelStream
    Type="#_pixelstream_mjpeg_"
    URL="http://localhost:8090/stream"
    Timeout="5"
    X="80" Y="30"
    Width="640" Height="480"
/>
```

### PixelStream Attributes

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Type` | string | Yes | Stream type. Use `#_pixelstream_mjpeg_` for MJPEG |
| `URL` | string | Yes | The HTTP endpoint serving the MJPEG stream |
| `Timeout` | integer | No | Connection timeout in seconds |
| `X` | double | No | Horizontal position |
| `Y` | double | No | Vertical position |
| `Width` | double | No | Display width in pixels or virtual units |
| `Height` | double | No | Display height in pixels or virtual units |

Standard positioning and alignment attributes (ParentAlignX, ParentAlignY, LocalAlignX, LocalAlignY) are also supported, just as with other visual elements.

### MJPEG Mode

In MJPEG mode (`#_pixelstream_mjpeg_`), dcapp connects to an HTTP endpoint that serves a `multipart/x-mixed-replace` stream of JPEG frames. This is a common format supported by many IP cameras, video servers, and custom applications.

### Setup and Usage

The MJPEG stream source must be running before dcapp connects to it. For the included sample, a Python test server is provided:

```bash
# Terminal 1: Start the MJPEG server
cd samples/pixelstream-mjpeg
python3 server.py

# Terminal 2: Launch the dcapp display
dcapp samples/pixelstream-mjpeg/pixelstream-mjpeg.xml
```

### Example

From `samples/pixelstream-mjpeg/pixelstream-mjpeg.xml`:

```xml
<DCAPP>
    <Window Title="PixelStream MJPEG Demo" Width="800" Height="600"
            VirtualWidth="800" VirtualHeight="600">

        <!-- Background -->
        <Rectangle Width="800" Height="600" FillColor="0.06 0.06 0.09"/>

        <!-- Title -->
        <Text ParentAlignX="#_align_center_" Y="570"
              LocalAlignX="#_align_center_" Size="22"
              FillColor="0.9 0.9 1">PixelStream MJPEG</Text>

        <!-- MJPEG Stream -->
        <PixelStream
            Type="#_pixelstream_mjpeg_"
            URL="http://localhost:8090/stream"
            Timeout="5"
            X="80" Y="30"
            Width="640" Height="480"
        />

        <!-- Instructions -->
        <Text ParentAlignX="#_align_center_" Y="8"
              LocalAlignX="#_align_center_" Size="10"
              FillColor="0.4 0.4 0.5">Start server first: python3 server.py</Text>
    </Window>
</DCAPP>
```

---

## See Also

- [Variables](variables.md) -- Declaring and using variables that integrate with external data
- [Logic Files](logic.md) -- Custom C/C++ code for advanced data processing and integration
- [Samples Index](samples.md) -- Runnable examples including integration demos
