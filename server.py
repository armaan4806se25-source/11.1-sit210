
from flask import Flask, request, jsonify, render_template
import time

app = Flask(__name__)

sensor_data = {
    "waterLevel": 0,
    "rainADC": 0,
    "rainDetected": False,
    "flowRate": 0,
    "floodStatus": "Waiting for Arduino",
    "pumpOn": False,
    "buzzerOn": False,
    "gateOpen": False,
    "lastUpdate": 0
}


@app.route("/")
def dashboard():
    return render_template("index.html")


@app.route("/api/update", methods=["POST"])
def update_data():
    global sensor_data

    data = request.get_json(silent=True)

    if not isinstance(data, dict):
        return jsonify({"error": "Invalid JSON"}), 400

    required = [
        "waterLevel",
        "rainADC",
        "rainDetected",
        "flowRate",
        "floodStatus",
        "pumpOn",
        "buzzerOn",
        "gateOpen"
    ]

    if not all(key in data for key in required):
        return jsonify({"error": "Missing sensor data"}), 400

    try:
        sensor_data.update({
            "waterLevel": float(data["waterLevel"]),
            "rainADC": int(data["rainADC"]),
            "rainDetected": bool(data["rainDetected"]),
            "flowRate": float(data["flowRate"]),
            "floodStatus": str(data["floodStatus"]),
            "pumpOn": bool(data["pumpOn"]),
            "buzzerOn": bool(data["buzzerOn"]),
            "gateOpen": bool(data["gateOpen"]),
            "lastUpdate": time.time()
        })
    except (ValueError, TypeError):
        return jsonify({"error": "Invalid sensor values"}), 400

    return jsonify({"message": "Data received"}), 200


@app.route("/api/data")
def get_data():
    result = sensor_data.copy()

    result["connected"] = (
        time.time() - sensor_data["lastUpdate"] <= 5
        and sensor_data["lastUpdate"] != 0
    )

    return jsonify(result)


if __name__ == "__main__":
    print("Smart Flood Drain Monitoring Server Started")
    app.run(host="0.0.0.0", port=5000, debug=False)
