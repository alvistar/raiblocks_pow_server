class PowBackend:
    def __init__(self):
        pass

    def send_request(self, hash):
        print(hash)
        result = input("{}:".format(hash))
        return result