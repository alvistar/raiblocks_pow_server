import threading
import time
import redis


class PowBackend:
    def __init__(self):
        self.r = redis.StrictRedis(host='localhost')
        self.p = self.r.pubsub()
        self.p.subscribe('requests')
        self.t1 = threading.Thread(target=self.worker)
        self.t1.start()

    def worker(self):
        while True:
            message = self.p.get_message()
            if message:
                print(message)

            time.sleep(0.001)


p = PowBackend()

while True:
    pass
