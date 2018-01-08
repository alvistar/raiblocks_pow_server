from flask import Flask
from flask import jsonify, request
from usb_backend import PowBackend

app = Flask(__name__)

pb = PowBackend()


@app.route('/', methods=['POST'])
def post_json_handler():
    print(request.is_json)
    content = request.get_json(force=True)
    print(content)
    return jsonify(work=pb.send_request(content['hash']))


app.run(host='0.0.0.0', port=8090)
