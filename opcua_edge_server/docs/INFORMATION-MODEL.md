# OPC UA Information Model — CNC Machine

Namespace: `urn:cnc-edge:CncMachine`  (ns index = 2 on a default server)

Object node: `ns=2;i=1000`  browse name `CncMachine` under Objects folder.

## Variable Nodes

| NodeId | Browse name | Data type | Engineering unit | Min sampling interval | Model |
|---|---|---|---|---|---|
| `ns=2;i=1001` | `Spindle.Temperature_C` | Float | °C (`unitId=4408652`) | 1 ms | T(t) = 22 + 45·(1−e^{−t/120}) + N(0,0.05²) |
| `ns=2;i=1002` | `Spindle.Torque_Nm`      | Float | N·m | 1 ms | 12·(1+0.1·sin(4πt)) + N(0,0.04) |
| `ns=2;i=1003` | `Spindle.SpeedRPM`       | Float | RPM | 1 ms | 3000 + N(0,25) |
| `ns=2;i=1004` | `Vibration.X_mms`        | Float | mm/s | 1 ms | 2.5·sin(100πt) + N(0,0.01) |
| `ns=2;i=1005` | `Vibration.Y_mms`        | Float | mm/s | 1 ms | 2.5·sin(100πt + 2π/3) + N(0,0.01) |
| `ns=2;i=1006` | `Vibration.Z_mms`        | Float | mm/s | 1 ms | 2.5·sin(100πt + 4π/3) + N(0,0.01) |
| `ns=2;i=1007` | `Axis.X_mm`              | Float | mm | 1 ms | 50·sin(0.2πt) |
| `ns=2;i=1008` | `Axis.Y_mm`              | Float | mm | 1 ms | 50·sin(0.2πt + π/6) |
| `ns=2;i=1009` | `Axis.Z_mm`              | Float | mm | 1 ms | 50·sin(0.2πt + π/3) |

## Access Level

All nodes: `UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_STATUSCODE`  
Write is not exposed — sensor values are read-only simulation outputs.

## Update Rate

The timerfd thread fires at 1 kHz (`CLOCK_MONOTONIC`, 1 ms interval). All 9 values
are written atomically to `g_node_values[NUM_NODES]` each tick. OPC UA clients
can subscribe with `SamplingInterval ≥ 1 ms` and `PublishingInterval ≥ 10 ms`.
