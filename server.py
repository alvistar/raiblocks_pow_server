import redis

from flask import Flask
from flask import jsonify, request
from usb_backend import PowBackend

app = Flask(__name__)

pb = PowBackend()

r = redis.StrictRedis(host='localhost')
p = r.pubsub(ignore_subscribe_messages=True)


@app.route('/', methods=['POST'])
def post_json_handler():
    print(request.is_json)
    content = request.get_json(force=True)
    print(content)
    p.subscribe(content['hash'])
    r.publish('requests', content['hash'])
    while True:
        message = p.get_message()
        if message:
            print(message)
            break

    return jsonify(work=message['data'].hex())


if __name__ == "__main__":
    app.run(host='0.0.0.0', port=8090)
