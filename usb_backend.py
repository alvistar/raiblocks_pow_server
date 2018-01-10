import config
import redis
import serial
import threading


class PowBackend:
    def __init__(self):
        self.ser = serial.Serial(config.SERIAL, 115200)
        print(self.ser.name)

        self.r = redis.StrictRedis(host='localhost')
        self.p = self.r.pubsub(ignore_subscribe_messages=True)
        self.p.subscribe('requests')

        self.t = threading.Thread(target=self.worker,  daemon=True)
        self.t.start()

    def worker(self):
        for message in self.p.listen():
            print(message)
            self.ser.write(b'12345678')
            # print('Written random')
            payload = bytes.fromhex(message['data'].decode('ascii'))
            print("Hash:{}\nLen:{}".format(payload.hex(), len(payload)))
            self.ser.write(payload)
            # print('Written bytes')
            s = self.ser.read(20)
            print("Output: {}".format(s.hex()))
            self.r.publish(message['data'], s[8:16][::-1])

    def send_request(self, input_hash):
        self.ser.write(input_hash.encode('ascii'))
        result = self.ser.readline()
        return result.decode('ascii')


if __name__ == "__main__":
    p = PowBackend()

    while True:
        pass
