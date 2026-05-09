import socket
import struct
import sys


# Must match the C definitions exactly.
#
# struct entry {
#     double latency;
#     uint64_t dropped;
#     uint64_t duplicate;
# };
#
# struct segment {
#     struct entry entries[ENTRY_COUNT];
# };
#
# Little-endian layout assumed.

ENTRY_COUNT = 1024  # change to match your config.h
RECEIVER_PORT = 8329  # change to match your config.h

ENTRY_STRUCT = struct.Struct("<dQQ")
ENTRY_SIZE = ENTRY_STRUCT.size

SEGMENT_SIZE = ENTRY_SIZE * ENTRY_COUNT


def recv_exact(sock: socket.socket, size: int) -> bytes:
    data = bytearray()

    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise ConnectionError("peer disconnected")

        data.extend(chunk)

    return bytes(data)


def decode_segment(data: bytes):
    for i in range(ENTRY_COUNT):
        offset = i * ENTRY_SIZE
        latency, dropped, duplicate = ENTRY_STRUCT.unpack_from(data, offset)

        yield {
            "index": i,
            "latency_ns": latency,
            "dropped": dropped,
            "duplicate": duplicate,
        }


def main():
    bind_addr = "0.0.0.0"

    if len(sys.argv) > 1:
        bind_addr = sys.argv[1]

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    server.bind((bind_addr, RECEIVER_PORT))
    server.listen(1)

    print(f"listening on {bind_addr}:{RECEIVER_PORT}")

    conn, addr = server.accept()

    print(f"connection from {addr[0]}:{addr[1]}")

    segment_number = 0

    try:
        while True:
            raw_segment = recv_exact(conn, SEGMENT_SIZE)

            print(f"\nsegment {segment_number}")

            for entry in decode_segment(raw_segment):
                print(
                    f"[{entry['index']:04d}] "
                    f"latency={entry['latency_ns']:.0f} ns "
                    f"dropped={entry['dropped']} "
                    f"duplicate={entry['duplicate']}"
                )

            segment_number += 1

    except KeyboardInterrupt:
        print("\nshutting down")

    except ConnectionError as e:
        print(f"\nconnection closed: {e}")

    finally:
        conn.close()
        server.close()


if __name__ == "__main__":
    main()