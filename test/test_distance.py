import serial, csv, statistics, time
import matplotlib.pyplot as plt

PORT = "COM5"
BAUD = 115200
DISTANCES = [5, 10, 15, 20, 25, 30, 35, 40]
N = 20              # fresh readings per distance
DRAIN_SECONDS = 1.5 # time spent throwing away old readings after Enter

def send(cmd):
    ser.write((cmd + "\n").encode())

def drain(seconds):
    """Read and discard everything for `seconds`, flushing all old data."""
    end = time.time() + seconds
    thrown = 0
    while time.time() < end:
        if ser.readline():
            thrown += 1
    return thrown

def collect(n):
    """Collect n fresh readings. Returns list of (angle, cm, us)."""
    got = []
    start = time.time()
    while len(got) < n and time.time() - start < 15:
        line = ser.readline().decode(errors="ignore").strip()
        parts = line.split(",")
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
send("V")     # make sure it is streaming
send("H90")   # hold straight ahead
time.sleep(1)
print("Ready.")

rows, means = [], []
for d in DISTANCES:
    input(f"\nPlace target at {d} cm, then press Enter...")
    thrown = drain(DRAIN_SECONDS)
    print(f"  (discarded {thrown} old lines)")

    readings = collect(N)
    if len(readings) < 2:
        print(f"  Not enough readings at {d} cm, skipping")
        continue

    for i, (a, cm, us) in enumerate(readings, 1):
        flag = "" if a == 90 else "   <-- not holding 90!"
        print(f"  {i:2d}: angle {a:3d}, {cm:6.2f} cm, {us:5d} us{flag}")

    cms = [cm for _, cm, _ in readings]
    m, sd = statistics.mean(cms), statistics.stdev(cms)
    means.append((d, m))
    rows += [(d, cm) for cm in cms]
    print(f"{d} cm -> mean {m:.2f} cm, error {m - d:+.2f} cm, sd {sd:.2f}")

send("S")     # resume sweeping
ser.close()

with open("test/data/distance.csv", "w", newline="") as f:
    w = csv.writer(f)
    w.writerow(["true_cm", "measured_cm"])
    w.writerows(rows)

xs = [d for d, _ in means]
ys = [m for _, m in means]
plt.plot(xs, xs, "--", label="ideal")
plt.plot(xs, ys, "o-", label="measured (mean)")
plt.xlabel("True distance (cm)")
plt.ylabel("Measured distance (cm)")
plt.title("HC-SR04 distance accuracy")
plt.legend(); plt.grid(True)
plt.savefig("test/data/distance.png", dpi=150)
plt.show()