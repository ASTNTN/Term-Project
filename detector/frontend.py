import os
from matplotlib.animation import FuncAnimation
import numpy as np
import matplotlib.pyplot as plt
import sys

WINDOW = 1000

dtype = np.dtype([
	("latency",   np.float64),
	("dropped",   np.uint64),
	("duplicate", np.uint64),
])

BYTES_PER_ENTRY = dtype.itemsize

def load_data(file):
	if not os.path.exists(file):
		print(f"{file} does not exist!")
		return None
	
	try:
		size = os.path.getsize(file)

		usable = size - (size % BYTES_PER_ENTRY)
		if usable < BYTES_PER_ENTRY:
			return None

		entries = usable // BYTES_PER_ENTRY
		take = min(entries, WINDOW)

		offset = (entries - take) * BYTES_PER_ENTRY

		with open(file, "rb") as f:
			f.seek(offset)
			data = f.read(keep)

		if not data:
			return None
		
		return np.frombuffer(data, dtype=dtype)
	
	except Exception:
		return None
	

def main(file):
	plt.style.use("dark_background")
	fig = plt.figure(figsize=(16, 9))
	gs = fig.add_gridspec(3, 3, height_ratios=[2, 1, 0.5])
	
	ax_latency = fig.add_subplot(gs[0, :])
	ax_drop	= fig.add_subplot(gs[1, 0])
	ax_dup	 = fig.add_subplot(gs[1, 1])
	ax_stats   = fig.add_subplot(gs[1, 2])
	ax_stats.axis("off")
	
	fig.suptitle(
		"Real-Time Network Latency Monitor",
		fontsize=15,
		fontweight="bold"
	)
	
	(latency_line,) = ax_latency.plot([], [], linewidth=1.5, color="lightblue")
	(drop_line,)	= ax_drop.plot([], [], linewidth=1.5, color="orange")
	(dup_line,)	 = ax_dup.plot([], [], linewidth=1.5, color="red")
	spike_scatter = None
	
	ax_latency.set_title("Latency Over Time")
	ax_latency.set_ylabel("Latency (ns)")
	ax_latency.set_xlim(0, WINDOW)
	ax_latency.grid(True, alpha=0.25)
	ax_drop.set_title("Dropped Packets")
	ax_drop.set_xlim(0, WINDOW)
	ax_drop.grid(True, alpha=0.25)
	ax_dup.set_title("Duplicate Packets")
	ax_dup.set_xlim(0, WINDOW)
	ax_dup.grid(True, alpha=0.25)
	
	def update(_):
		global spike_scatter
	
		data = load_data()
		if data is None or len(data) == 0:
			return
	
		latency   = data["latency"]
		dropped   = data["dropped"]
		duplicate = data["duplicate"]
	
		x = np.arange(len(latency))
	
		#Latency 
		latency_line.set_data(x, latency)
		y_max = max(np.percentile(latency, 95), 1)
		ax_latency.set_ylim(0, y_max * 1.2)
	
		#Spikes
		mean = np.mean(latency)
		threshold = mean * 1.5
		spikes = latency > threshold
	
		if spike_scatter is not None:
			spike_scatter.remove()
	
		spike_scatter = ax_latency.scatter(
			x[spikes],
			latency[spikes],
			s=30,
			color="white"
		)
	
		#Dropped
		drop_line.set_data(x, dropped)
		ax_drop.set_ylim(0, max(np.max(dropped, initial=1) + 1, 1))
		
		#Duplicate
		dup_line.set_data(x, duplicate)
		ax_dup.set_ylim(0, max(np.max(duplicate, initial=1) + 1, 1))
	
		#Stats
		stats = (
			f"Window Size: {len(data)} samples\n"
			f"-----------------------------\n"
			f"Avg Latency: {mean:.0f} ns\n"
			f"Max Latency: {np.max(latency):.0f} ns\n"
			f"Std Dev	: {np.std(latency):.0f} ns\n"
			f"\n"
			f"Dropped	: {np.sum(dropped)}\n"
			f"Duplicates : {np.sum(duplicate)}\n"
			f"\n"
			f"Status	 : {'UNSTABLE' if np.any(latency > threshold) else 'STABLE'}"
		)
	
		ax_stats.clear()
		ax_stats.axis("off")
		ax_stats.text(0, 1, stats, fontsize=11, family="monospace",verticalalignment="top")

	animation = FuncAnimation(fig, update, interval=200, cache_frame_data=False)
	plt.tight_layout()
	plt.show()

if __name__ == "__main__":
	main(sys.argv[1])