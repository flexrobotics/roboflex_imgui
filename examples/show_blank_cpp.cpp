#include <iostream>
#include <roboflex_imgui/imgui_nodes.h>

int main() {

    auto onedtv = roboflex::imgui::OneDTV(
        "data",                 // name of key of data tensor in message
        8,                      // number of messages to retain (to draw)
        true,                   // center on zero
        {640, 220},             // initial size
        {100, -1},              // initial position
        "Audio Television",     // node name
        true                    // debug
    );

    std::cout << onedtv.to_string() << std::endl;

    // will block until window is closed
    onedtv.run();

    return 0;
}