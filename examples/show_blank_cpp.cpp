#include <iostream>
#include <roboflex_imgui/oned_television.h>
#include <roboflex_imgui/metrics_television.h>

int main() {

    // auto onedtv = roboflex::imgui::OneDTV(
    //     "data",                 // name of key of data tensor in message
    //     8,                      // number of messages to retain (to draw)
    //     true,                   // center on zero
    //     {640, 220},             // initial size
    //     {100, -1},              // initial position
    //     "Audio Television",     // node name
    //     true                    // debug
    // );

    auto onedtv = roboflex::imgui::MetricsTelevision(
        "MetricsTelevision",    // window title
        {1580, 720},            // initial size
        {100, -1},              // initial position
        "Metrics Television",   // node name
        true                    // debug
    );

    std::cout << onedtv.to_string() << std::endl;

    // will block until window is closed
    onedtv.run();

    return 0;
}