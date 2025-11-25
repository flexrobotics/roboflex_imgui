#include <roboflex_core/core_nodes/message_printer.h>
#include <roboflex_core/core_nodes/map_fun.h>
#include <roboflex_core/core_nodes/callback_fun.h>
#include <roboflex_imgui/oned_television.h>
#include <roboflex_transport_mqtt/mqtt_nodes.h>
#include <roboflex_transport_zmq/zmq_nodes.h>
#include <roboflex_core/core_messages/core_messages.h>

int main() {

    // auto mqtt_context = roboflex::transportmqtt::MakeMQTTContext();
    // auto mqtt_sub = roboflex::transportmqtt::MQTTSubscriber(
    //     mqtt_context,
    //     "192.168.0.21",     // broker address
    //     1883,               // broker port
    //     "imu"               // broker topic
    // );

    auto zmq_context = roboflex::transportzmq::MakeZMQContext();
    auto zmq_sub = roboflex::transportzmq::ZMQSubscriber(
        zmq_context,
        "tcp://localhost:5555", // bind address
        "ZMQSubscriber",        // node name
        8                       // queue size
    );

    auto msg_printer = roboflex::nodes::MessagePrinter();

    auto tensor_printer = roboflex::nodes::CallbackFun(
        [](roboflex::core::MessagePtr m) {
            auto mtens = roboflex::core::TensorMessage<float, 2>(*m, "data");
            std::cout << mtens.to_string() << std::endl;
        }
    );

    // auto data_transformer = roboflex::nodes::MapFun(
    //     [](roboflex::core::MessagePtr m) {
    //         std::cout << "RECEIVED " << m->to_string() << std::endl;
    //         auto v = m->root_val("q").AsVector();
    //         xt::xarray<double> d { 
    //             v[0].AsDouble(),
    //             v[1].AsDouble(),
    //             v[2].AsDouble(),
    //             v[3].AsDouble() 
    //         };
    //         auto data = xt::expand_dims(d, 1);
    //         return roboflex::core::TensorMessage<double, 2>::Ptr(data, "TensorMessage", "data");
    //     },
    //     "data_transformer"
    // );

    auto onedtv = roboflex::imgui::OneDTV(
        "data",                 // window title
        "data",                 // name of key of data tensor in message
        500,                    // number of messages to retain (to draw)
        roboflex::imgui::OneDTV::PlotStyle::Scatter, // plot style
        0.1f,                   // marker size
        true,                   // center on zero
        {640, 350},             // initial size
        {100, -1},              // initial position
        150,                    // plot height
        false,                  // annotate lead
        "data television",      // node name
        false                   // debug
    );

    //mqtt_sub > data_transformer > onedtv;
    zmq_sub > msg_printer > tensor_printer > onedtv;

    zmq_sub.start();
    onedtv.run();
    zmq_sub.stop();

    return 0;
}