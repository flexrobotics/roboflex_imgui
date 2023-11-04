#ifndef ROBOFLEX_IMPLOT_NODE__H
#define ROBOFLEX_IMPLOT_NODE__H

#include <mutex>
#include <SDL2/SDL.h>
#include "roboflex_core/core.h"

namespace roboflex {
namespace imgui {

using std::string, std::pair;

class IMPLOTNode: public core::RunnableNode {
public:
    IMPLOTNode(
        const string& window_title = "IMPLOTNode",
        const pair<int, int>& initial_size = {640, 220},
        const pair<int, int>& initial_pos = {-1, -1},
        const string& name = "IMPLOTNode",
        const bool debug = false);

protected:

    // child classes should override this
    virtual void draw() {}

    void child_thread_fn() override;

    string window_title;
    pair<int, int> initial_size = {640, 220};
    pair<int, int> initial_pos = {-1, -1};
    bool debug = false;
};

}  // namespace imgui
}  // namespace roboflex

#endif // ROBOFLEX_IMPLOT_NODE__H