import logging
import redis

from jsonschema import validate, ValidationError
from flask import Flask
from flask import jsonify, request, abort
from fpga_backend import PowBackend

schema = {
    "type": "object",
    "properties": {
        "hash": {"type": "string", "pattern": "[0-9A-Fa-f]{64}"}
    }
}

logging.basicConfig(level=logging.DEBUG)

app = Flask(__name__)

pb = PowBackend()

r = redis.StrictRedis(host='localhost')
p = r.pubsub(ignore_subscribe_messages=True)


@app.route('/', methods=['POST'])
def post_json_handler():
    content = request.get_json(force=True)
    logging.info('Received request:{}'.format(content))

    try:
        validate(content, schema)
    except ValidationError as e:
        logging.error('Received bad request:{}'.format(e))
        abort(400)

    p.subscribe(content['hash'])
    r.publish('requests', content['hash'])

    for message in p.listen():
        break

    logging.debug('Received message from PowBackend:{}'.format(message))
    logging.info('POW:{}'.format(message['data'].hex()))
    return jsonify(work=message['data'].hex())


if __name__ == "__main__":
    app.run(host='0.0.0.0', port=8090)
