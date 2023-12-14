#include <roboflex_imgui/oned_television.h>
#include <roboflex_transport_zmq/zmq_nodes.h>

int main() {

    auto onedtv = roboflex::imgui::OneDTV(
        "audio",                // window title
        "data",                 // name of key of data tensor in message
        8,                      // number of messages to retain (to draw)
        roboflex::imgui::OneDTV::PlotStyle::Line, // plot style
        0.1f,                   // marker size
        true,                   // center on zero
        {640, 220},             // initial size
        {100, -1},              // initial position
        -1,                     // plot height
        false,                  // annotate lead
        "Audio Television",     // node name
        false                   // debug
    );

    auto zmq_context = roboflex::transportzmq::MakeZMQContext();
    auto zmq_sub = roboflex::transportzmq::ZMQSubscriber(
        zmq_context,
        "tcp://localhost:5553", // bind address
        "ZMQSubscriber",        // node name
        8                       // queue size
    );

    zmq_sub > onedtv;

    zmq_sub.start();
    onedtv.run();
    zmq_sub.stop();

    return 0;
}