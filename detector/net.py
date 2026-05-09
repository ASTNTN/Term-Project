from collections import deque
import matplotlib.pyplot as plt
import socket
import struct
import sys
import time

ENTRY_COUNT = 1024
RECEIVER_PORT = 8329

ENTRY_STRUCT = struct.Struct("<dQQ")
ENTRY_SIZE = ENTRY_STRUCT.size

SEGMENT_SIZE = ENTRY_SIZE * ENTRY_COUNT

HISTORY = 10000

def recv_exact(socket, size):
	data = bytearray()

	while len(data) < size:
		chunk = socket.recv(size - len(data))

		if not chunk:
			raise ConnectionError("peer disconnected")

		data.extend(chunk)

	return bytes(data)


def decode_segment(data: bytes):
	for i in range(ENTRY_COUNT):
		offset = i * ENTRY_SIZE

		latency, dropped, duplicate = ENTRY_STRUCT.unpack_from(
			data,
			offset,
		)

		yield latency, dropped, duplicate


def main(sink):
	bind_addr = "0.0.0.0"

	server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	server.bind((bind_addr, RECEIVER_PORT))
	server.listen(1)

	print(f"listening on {bind_addr}:{RECEIVER_PORT}")

	conn, addr = server.accept()

	print(f"connection from {addr[0]}:{addr[1]}")

	plt.ion()

	fig, ax = plt.subplots(figsize=(12, 6))

	latency_history = deque(maxlen=HISTORY)
	dropped_history = deque(maxlen=HISTORY)
	duplicate_history = deque(maxlen=HISTORY)

	x_history = deque(maxlen=HISTORY)

	line_latency, = ax.plot([], [], label="Latency (ns)")

	ax.set_title("Packet Latency")
	ax.set_xlabel("Packet Index")
	ax.set_ylabel("Latency (ns)")

	ax.legend()

	packet_counter = 0
	segment_number = 0

	try:
		while True:
			raw_segment = recv_exact(conn, SEGMENT_SIZE)
			sink.write(raw_segment)
			sink.flush()

			for latency_ns, dropped, duplicate in decode_segment(raw_segment):
				x_history.append(packet_counter)
				latency_history.append(latency_ns)

				dropped_history.append(dropped)
				duplicate_history.append(duplicate)

				packet_counter += 1

			x = list(x_history)
			y = list(latency_history)

			line_latency.set_data(x, y)

			ax.relim()
			ax.autoscale_view()

			plt.draw()
			plt.pause(0.001)

			segment_number += 1

	except KeyboardInterrupt:
		print("\nshutting down")

	except ConnectionError as e:
		print(f"\nconnection closed: {e}")

	finally:
		conn.close()
		server.close()


if __name__ == "__main__":
	sink_name = f"data-{time.time()}.hex"
	sink = open(sink_name, "ab")

	print(f"saving to {sink_name}")

	main(sink)
