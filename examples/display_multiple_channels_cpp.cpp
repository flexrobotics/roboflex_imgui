#include <roboflex_core/core_nodes/message_printer.h>
#include <roboflex_core/core_nodes/map_fun.h>
#include <roboflex_imgui/oned_television.h>
#include <roboflex_transport_mqtt/mqtt_nodes.h>

int main() {

    auto mqtt_context = roboflex::transportmqtt::MakeMQTTContext();
    auto mqtt_sub = roboflex::transportmqtt::MQTTSubscriber(
        mqtt_context,
        "192.168.0.21",     // broker address
        1883,               // broker port
        "imu"               // broker topic
    );

    auto data_transformer = roboflex::nodes::MapFun(
        [](roboflex::core::MessagePtr m) {
            auto v = m->root_val("q").AsVector();
            xt::xarray<double> d { 
                v[0].AsDouble(),
                v[1].AsDouble(),
                v[2].AsDouble(),
                v[3].AsDouble() 
            };
            // xt::xarray<double> d { 
            //     v[0].AsDouble()
            // };
            auto data = xt::expand_dims(d, 1);
            //std::cout << "Signalling data:" << xt::adapt(data.shape()) << "  " << data << std::endl;
            return roboflex::core::TensorMessage<double, 2>::Ptr(data, "TensorMessage", "data");
        },
        "data_transformer"
    );

    auto onedtv = roboflex::imgui::OneDTV(
        "data",                 // window title
        "data",                 // name of key of data tensor in message
        200,                    // number of messages to retain (to draw)
        roboflex::imgui::OneDTV::PlotStyle::Line, // plot style
        0.1f,                   // marker size
        false,                  // center on zero
        {640, 640},             // initial size
        {100, -1},              // initial position
        140,                    // plot height
        true,                   // annotate lead
        "data television",      // node name
        false                   // debug
    );

    //auto printer = roboflex::nodes::MessagePrinter();
    //mqtt_sub > data_transformer > printer > onedtv;
    mqtt_sub > data_transformer > onedtv;

    mqtt_sub.start();
    onedtv.run();
    mqtt_sub.stop();

    return 0;
}