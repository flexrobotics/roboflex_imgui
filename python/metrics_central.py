import argparse
import roboflex as rfx
import roboflex.transport.mqtt as rtm
import roboflex.imgui as rgu


def get_metrics_mqtt_publisher(
    broker_address: str,
    broker_port: int = 1883,
    metrics_topic: str = "metrics",
):
    return rtm.MQTTPublisher(
        mqtt_context=rtm.MQTTContext(),
        broker_address=broker_address,
        broker_port=broker_port,
        topic_name=metrics_topic)


def get_metrics_mqtt_subscriber(
    broker_address: str,
    broker_port: int = 1883,
    metrics_topic: str = "metrics",
):
    return rtm.MQTTSubscriber(
        mqtt_context=rtm.MQTTContext(),
        broker_address=broker_address,
        broker_port=broker_port,
        topic_name=metrics_topic)


def get_metrics_visualizer(
    broker_address: str,
    broker_port: int = 1883,
    metrics_topic: str = "metrics",
):
    title = f"Metrics Central Mqtt: {broker_address}:{broker_port}/{metrics_topic}"
    return rgu.MetricsTelevision(window_title = title)


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("-b", "--broker_address", type=str)
    parser.add_argument("-p", "--broker_port", type=int, default=1883)
    parser.add_argument("-t", "--metrics_topic", type=str, default="metrics")
    args = parser.parse_args()

    try:
        metrics_subscriber = get_metrics_mqtt_subscriber(args.broker_address, args.broker_port, args.metrics_topic)
    except:
        print(f"Could not connect to an mqtt broker at {args.broker_address}:{args.broker_port}/{args.metrics_topic}")

    metrics_visualizer = get_metrics_visualizer(args.broker_address, args.broker_port, args.metrics_topic)
    metrics_subscriber > metrics_visualizer
    metrics_subscriber.start()
    metrics_visualizer.run() # will block until the window is closed
    metrics_subscriber.stop()