#include <roboflex_imgui/imgui_nodes.h>
#include <roboflex_transport_zmq/zmq_nodes.h>

int main() {

    auto onedtv = roboflex::imgui::OneDTV(
        "data",                 // name of key of data tensor in message
        8,                      // number of messages to retain (to draw)
        true,                   // center on zero
        {640, 220},             // initial size
        {100, -1},              // initial position
        "Audio Television",     // node name
        false                   // debug
    );

    auto zmq_context = roboflex::transportzmq::MakeZMQContext();
    auto zmq_sub = roboflex::transportzmq::ZMQSubscriber(
        zmq_context,
        "tcp://127.0.0.1:5555", // bind address
        "ZMQSubscriber",        // node name
        2                       // queue size
    );

    zmq_sub > onedtv;

    onedtv.start();
    zmq_sub.start();

    sleep(100);

    onedtv.stop();
    zmq_sub.stop();

    return 0;
}