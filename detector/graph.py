import numpy as np
import matplotlib.pyplot as plt

dtype = np.dtype([
    ("latency",   np.float64),
    ("dropped",   np.uint64),
    ("duplicate", np.uint64),
])

def load_output(path):
    with open(path, "rb") as f:
        data = f.read()

    values = np.frombuffer(data, dtype=dtype)

    return values

def main():
    path = "output.hex"
    data = load_output(path)

    latency   = data["latency"]
    dropped   = data["dropped"]
    duplicate = data["duplicate"]

    x = np.arange(len(latency))
    
    fig, axs = plt.subplots(3, 1, figsize=(12, 8), sharex=True)
    
    axs[0].plot(x, latency, linewidth=1)
    axs[0].set_ylabel("Latency")
    axs[0].set_title("Latency over time")
    axs[0].grid(True)
    
    axs[1].plot(x, dropped, linewidth=1)
    axs[1].set_ylabel("Dropped")
    axs[1].set_title("Dropped packets")
    axs[1].grid(True)
    
    axs[2].plot(x, duplicate, linewidth=1)
    axs[2].set_ylabel("Duplicate")
    axs[2].set_title("Duplicate packets")
    axs[2].set_xlabel("Sample")
    axs[2].grid(True)
    
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()