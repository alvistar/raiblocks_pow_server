import serial


class PowBackend:
    def __init__(self):
        self.ser = serial.Serial('/dev/master')
        print(self.ser.name)

    def send_request(self, input_hash):
        self.ser.write(input_hash.encode('ascii'))
        result = self.ser.readline()
        return result.decode('ascii')
