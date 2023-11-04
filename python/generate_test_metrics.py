import time
import numpy as np
import roboflex as rfx
import roboflex.imgui.metrics_central as rmc

def generate_test_metrics():

    # publish metrics on mqtt
    metrics_pub = rmc.get_metrics_mqtt_publisher("127.0.0.1", 1883, "metrics")
    g = rfx.GraphRoot(metrics_pub)

    # create some dummy graph
    n1 = rfx.FrequencyGenerator(3.0, "node 1")
    n2 = rfx.MapFun(lambda _: {"d": np.ones((1000, 100), dtype=np.float32)}, "node 2")
    n3 = rfx.Node("node 3")
    g > n1 > n2 > n3

    # start the graph, with profiling on
    g.start(profile=True)
    time.sleep(100)
    g.stop()

if __name__ == "__main__":
    generate_test_metrics()