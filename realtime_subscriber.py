import time
import json
import threading
from queue import Queue
from pymongo import MongoClient
import paho.mqtt.client as mqtt

# ——— CONFIGURATION ———
MQTT_BROKER   = "broker.hivemq.com"
MQTT_PORT     = 1883
MQTT_TOPIC    = "nicla/data"

MONGO_URI     = "mongodb+srv://nicla_user:nicla@cluster0.fkzil.mongodb.net/?retryWrites=true&w=majority&appName=Cluster0"
MONGO_DB      = "test"
MONGO_COLL    = "portenta_stream"
BATCH_SIZE    = 1000
# ———————————

# Setup MongoDB client and insert queue
mongo = MongoClient(MONGO_URI)
collection = mongo[MONGO_DB][MONGO_COLL]
insert_queue = Queue()

# Background worker thread for MongoDB batch inserts
def mongo_worker():
    batch = []
    while True:
        doc = insert_queue.get()
        if doc is None:
            break  # Graceful shutdown
        batch.append(doc)

        if len(batch) >= BATCH_SIZE:
            try:
                collection.insert_many(batch)
                print(f"📥 Inserted batch of {BATCH_SIZE}, last timestamp: {batch[-1].get('timestamp')}")
            except Exception as e:
                print("❌ Insert failed:", e)
            batch.clear()

# Start the MongoDB insert worker thread
threading.Thread(target=mongo_worker, daemon=True).start()

# MQTT Callbacks
def on_connect(client, userdata, flags, rc):
    print("✅ MQTT connected (rc=%s). Subscribing to %s…" % (rc, MQTT_TOPIC))
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    try:
        samples = json.loads(msg.payload.decode())
        now = time.time()

        if isinstance(samples, list):
            for doc in samples:
                doc["ts"] = now  # Add arrival timestamp
                insert_queue.put(doc)
        else:
            print("⚠️ Expected JSON array, got:", type(samples))

    except Exception as e:
        print("❌ Error parsing MQTT message:", e)

# Setup MQTT client
mqttc = mqtt.Client()
mqttc.on_connect = on_connect
mqttc.on_message = on_message
mqttc.connect(MQTT_BROKER, MQTT_PORT, 60)
mqttc.loop_forever()
