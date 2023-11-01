#ifndef ROBOFLEX_IMGUI__H
#define ROBOFLEX_IMGUI__H

#include <deque>
#include <mutex>
#include <vector>
#include <SDL2/SDL.h>
#include "roboflex_core/core.h"
#include "roboflex_core/core_nodes/frequency_generator.h"

namespace roboflex {
namespace imgui {

using std::string, std::vector, std::pair, std::deque;

class IMPLOTNode: public core::RunnableNode {
public:
    IMPLOTNode(
        const pair<int, int>& initial_size = {640, 220},
        const pair<int, int>& initial_pos = {-1, -1},
        const string& name = "IMPLOTNode",
        const bool debug = false);

protected:

    // child classes should override this
    virtual void draw() {}

    void child_thread_fn() override;

    pair<int, int> initial_size = {640, 220};
    pair<int, int> initial_pos = {-1, -1};
    bool debug = false;
};

class OneDTV: public IMPLOTNode {
public:
    OneDTV(
        const string& data_key = "data",
        const unsigned int sample_size = 4,
        const bool center_zero = true,
        const pair<int, int>& initial_size = {640, 220},
        const pair<int, int>& initial_pos = {-1, -1},
        const string& name = "OneDTV",
        const bool debug = false);
    
    virtual ~OneDTV();

    void receive(MessagePtr m) override;

    string to_string() const override;

protected:

    void draw() override;
    void populate_values(vector<vector<double>>& values) const;

    string data_key = "data";
    unsigned int sample_size = 4;
    bool center_zero = true;

    vector<pair<double, double>> graph_limits;
    deque<pair<double, uint32_t>> timestamped_counts;

    mutable std::recursive_mutex data_queue_mutex;
    deque<core::MessagePtr> data_queue;
};

}  // namespace imgui
}  // namespace roboflex

#endif // ROBOFLEX_IMGUI__H
