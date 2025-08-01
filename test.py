import socket
import threading
import time
import random


def test(index):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(("127.0.0.1", 1024))
        print(f"[Client {index}] Connected")
        s.send(b'test'+str(index).encode())
        # Keep the connection open to test semaphore blocking
        print(s.recv(1056))
        time.sleep(random.randrange(0, 10))
        s.close()
        print(f"[Client {index}] Disconnected")
    except Exception as e:
        print(f"[Client {index}] Error: {e}")


threads = []

for x in range(16):
    t = threading.Thread(target=test, args=(x,))
    threads.append(t)
    t.start()

# Optionally wait for all threads to finish
for t in threads:
    t.join()
