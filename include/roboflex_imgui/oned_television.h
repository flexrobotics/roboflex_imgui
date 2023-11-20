#ifndef ROBOFLEX_ONE_D_TELEVISION__H
#define ROBOFLEX_ONE_D_TELEVISION__H

#include <deque>
#include <mutex>
#include "implot_node.h"

namespace roboflex {
namespace imgui {

using std::string, std::pair, std::deque, core::MessagePtr;

class OneDTV: public IMPLOTNode {
public:

    enum class PlotStyle {
        Line,
        Scatter
    };

    OneDTV(
        const string& window_title = "OneDTV",
        const string& data_key = "data",
        const unsigned int sample_size = 4,
        const PlotStyle plot_style = PlotStyle::Line,
        const float marker_size = 0.1f,
        const bool center_zero = true,
        const pair<int, int>& initial_size = {640, 220},
        const pair<int, int>& initial_pos = {-1, -1},
        const string& name = "OneDTV",
        const bool debug = false);
    
    virtual ~OneDTV();

    void receive(core::MessagePtr m) override;

    string to_string() const override;

protected:

    void draw() override;
    void populate_values(vector<vector<double>>& values) const;

    string data_key = "data";
    unsigned int sample_size = 4;
    PlotStyle plot_style = PlotStyle::Line;
    float marker_size = 0.1f;
    bool center_zero = true;

    vector<pair<double, double>> graph_limits;
    deque<pair<double, uint32_t>> timestamped_counts;

    mutable std::recursive_mutex data_queue_mutex;
    deque<core::MessagePtr> data_queue;
};

}  // namespace imgui
}  // namespace roboflex

#endif // ROBOFLEX_ONE_D_TELEVISION__H
