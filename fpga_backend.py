import config
import fpga_interface
import logging
import redis
import threading


class PowBackend:
    def __init__(self):
        fpga_interface.init_fpga()
        logging.info('Initialized fpga interface')

        self.r = redis.StrictRedis(host='localhost')
        self.p = self.r.pubsub(ignore_subscribe_messages=True)
        self.p.subscribe('requests')

        self.t = threading.Thread(target=self.
                                  worker,  daemon=True)
        self.t.start()

    def worker(self):
        for message in self.p.listen():
            logging.debug('Got pow request:{}'.format(message))
            result = fpga_interface.do_pow(message['data'])
            logging.debug("FPGA Output: {}".format(result))
            self.r.publish(message['data'], result[8:16])

if __name__ == "__main__":
    p = PowBackend()

    while True:
        pass
