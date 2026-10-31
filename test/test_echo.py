import serial, statistics, time
import numpy as np
import dwfpy as dwf

PORT = "COM5"
BAUD = 115200
RATE = 1_000_000    # 1 MHz = 1 us resolution
N = 50
TH = 1.5            # volts: above this counts as "high"
DRAIN_SECONDS = 1.5

def send(cmd):
    ser.write((cmd + "\n").encode())

def drain(seconds):
    end = time.time() + seconds
    while time.time() < end:
        ser.readline()

def collect(n):
    got = []
    start = time.time()
    while len(got) < n and time.time() - start < 20:
        parts = ser.readline().decode(errors="ignore").strip().split(",")
        if len(parts) != 3:
            continue
        try:
            got.append((int(parts[0]), float(parts[1]), int(parts[2])))
        except ValueError:
            continue
    return got

# ---- start up ----
ser = serial.Serial(PORT, BAUD, timeout=0.5)
print("Waiting for the ESP32 to boot...")
time.sleep(3)
drain(1)
send("V")
send("H90")
time.sleep(1)

# 1) AD3 measures N echo pulses
print("Capturing with the AD3...")
ad3, peak = [], 0.0
with dwf.Device() as dev:
    scope = dev.analog_input
    scope[0].setup(range=10.0)
    scope.setup_edge_trigger(mode="normal", channel=0, slope="rising", level=TH)
    for _ in range(N):
        scope.single(sample_rate=RATE, buffer_size=16384, configure=True, start=True)
        v = np.array(scope[0].get_data())
        peak = max(peak, v.max())
        ad3.append((v > TH).sum() / RATE * 1e6)   # pulse width in us

# 2) Firmware reports N fresh echo widths
print("Reading firmware values...")
drain(DRAIN_SECONDS)
readings = collect(N)
fw = [us for a, cm, us in readings]
not_held = sum(1 for a, _, _ in readings if a != 90)

send("S")
ser.close()

a_mean, f_mean = statistics.mean(ad3), statistics.mean(fw)
print(f"Peak ECHO voltage:  {peak:.2f} V")
print(f"AD3 mean echo:      {a_mean:.1f} us (sd {statistics.stdev(ad3):.1f})")
print(f"Firmware mean echo: {f_mean:.1f} us (sd {statistics.stdev(fw):.1f})")
print(f"Difference: {f_mean - a_mean:+.1f} us ({(f_mean - a_mean) / a_mean * 100:+.2f} %)")
if not_held:
    print(f"WARNING: {not_held} readings were not at 90 deg (hold mode not active)")