#include "roboflex_imgui/oned_television.h"
#include <implot.h>
#include "roboflex_core/serialization/flex_tensor_format.h"

namespace roboflex {
namespace imgui {


// --- Helper Functions ---

template <typename T>
void do_copy_data_into_values(MessagePtr m, const string& data_key, vector<vector<double>>& values)
{
    auto tensor_msg = core::TensorMessage<T, 2>(*m, data_key);
    auto tensor = tensor_msg.value();

    int num_sequences = tensor.shape()[0];
    int sequence_length = tensor.shape()[1];

    for (int i=0; i<num_sequences; i++) {
        // There's gotta be a direct-copy version, no?
        for (int j=0; j<sequence_length; j++) {
            values[i].push_back(tensor(i, j));
        }
    }
}

void copy_data_into_values(MessagePtr m, const string& data_key, vector<vector<double>>& values)
{
    auto dtype = m->root_map()[data_key].AsMap()["dtype"].AsInt8();
    switch (dtype) {
        case 0: do_copy_data_into_values<int8_t>(m, data_key, values); break;
        case 1: do_copy_data_into_values<int16_t>(m, data_key, values); break;
        case 2: do_copy_data_into_values<int32_t>(m, data_key, values); break;
        case 3: do_copy_data_into_values<int64_t>(m, data_key, values); break;
        case 4: do_copy_data_into_values<uint8_t>(m, data_key, values); break;
        case 5: do_copy_data_into_values<uint16_t>(m, data_key, values); break;
        case 6: do_copy_data_into_values<uint32_t>(m, data_key, values); break;
        case 7: do_copy_data_into_values<uint64_t>(m, data_key, values); break;
        //case 8: do_copy_data_into_values<intptr_t>(m, data_key, values); break;
        //case 9: do_copy_data_into_values<uintptr_t>(m, data_key, values); break;
        case 10: do_copy_data_into_values<float>(m, data_key, values); break;
        case 11: do_copy_data_into_values<double>(m, data_key, values); break;
        //case 12: do_copy_data_into_values<complex64>(m, data_key, values); break;
        //case 13: do_copy_data_into_values<complex128>(m, data_key, values); break;
    }
}


// -- OneDTV --

OneDTV::OneDTV(
    const string& window_title,
    const string& data_key,
    const unsigned int sample_size,
    const OneDTV::PlotStyle plot_style,
    const float marker_size,
    const bool center_zero,
    const pair<int, int>& initial_size,
    const pair<int, int>& initial_pos,
    const string& name,
    const bool debug):
        IMPLOTNode(window_title, initial_size, initial_pos, name, debug),
        data_key(data_key),
        sample_size(sample_size),
        plot_style(plot_style),
        marker_size(marker_size),
        center_zero(center_zero)
{

}

OneDTV::~OneDTV()
{    

}

string OneDTV::to_string() const
{
    return std::string("<OneDTV ") +
        " data_key: \"" + data_key + "\"" +
        " sample_size: " + std::to_string(sample_size) +
        " " + IMPLOTNode::to_string() + 
        ">";
}

void OneDTV::receive(MessagePtr m)
{
    const std::lock_guard<std::recursive_mutex> lock(data_queue_mutex);

    auto shape = serialization::tensor_shape(m->root_map()[data_key]);
    int sequence_length = shape[1];

    double t = core::get_current_time();
    double tm1 = t - 1.0;
    auto timestamped_count = pair<double, uint32_t>(t, sequence_length);
    timestamped_counts.push_back(timestamped_count);

    while (timestamped_counts[0].first <= tm1) {
        timestamped_counts.pop_front();
    }

    data_queue.push_back(m);
    if (data_queue.size() > sample_size) {
        data_queue.pop_front();
    }
}

void OneDTV::populate_values(vector<vector<double>>& values) const
{
    const std::lock_guard<std::recursive_mutex> lock(data_queue_mutex);

    if (data_queue.size() == 0) {
        return;
    }

    // Get the shape of the first message's tensor
    MessagePtr first = data_queue[0];
    auto shape = serialization::tensor_shape(first->root_map()[data_key]);
    int num_sequences = shape[0];
    int sequence_length = shape[1];

    // Pre-allocate vectors to hold values. Prolly inefficient...
    values.clear();
    for (int i=0; i<num_sequences; i++) {
        vector<double> d;
        d.reserve(sequence_length * data_queue.size());
        values.push_back(d);
    }

    // Copy the data from each message into the values
    // this is seems to be roughly 3x the cost of the 
    // above pre-allocation
    for (auto m: data_queue) {
        copy_data_into_values(m, data_key, values);
    }
}

void OneDTV::draw()
{
    vector<vector<double>> values;
    this->populate_values(values);

    uint32_t total_samples = 0;
    for (auto [t, c]: timestamped_counts) {
        total_samples += c;
    }

    auto x_flags = ImPlotAxisFlags_NoGridLines;
    auto y_flags = ImPlotAxisFlags_NoGridLines;

    if (ImGui::BeginTable("table", 1, 0, ImVec2(-1,0))) {

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("samples per second: %d", total_samples);

        size_t i = 0;
        for (const auto &v: values) {

            // compute graph limits
            const auto [min, max] = std::minmax_element(begin(v), end(v));
            if (i >= graph_limits.size()) {
                graph_limits.push_back({*min, *max});
            } else {
                graph_limits[i] = {
                    *min * 0.005 + graph_limits[i].first * 0.995, 
                    *max * 0.005 + graph_limits[i].second * 0.995
                };
            }
            double miny = center_zero ? (-1.0 * std::max(fabs(graph_limits[i].first), fabs(graph_limits[i].first))) : graph_limits[i].first;
            double maxy = center_zero ? ( 1.0 * std::max(fabs(graph_limits[i].second), fabs(graph_limits[i].second))) : graph_limits[i].second;

            auto flags = ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText | ImPlotAxisFlags_NoHighlight | ImPlotFlags_NoChild;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImPlot::SetNextAxesLimits(0, v.size(), miny, maxy, ImPlotCond_Always);
            //if (ImPlot::BeginPlot("", ImVec2(-1,100), flags)) {
            if (ImPlot::BeginPlot("", ImVec2(-1,-1), flags)) {
                ImPlot::SetupAxis(ImAxis_X1, NULL, x_flags);
                ImPlot::SetupAxis(ImAxis_Y1, NULL, y_flags);
                if (plot_style == OneDTV::PlotStyle::Scatter) {
                    ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, marker_size);
                    ImPlot::PlotScatter("##noid", v.data(), v.size());
                }
                else if (plot_style == OneDTV::PlotStyle::Line) {
                    ImPlot::PlotLine("##noid", v.data(), v.size());
                }
                //ImPlot::PlotScatter("##noid", v.data(), v.size());
                //ImPlot::PlotLine("##noid", v.data(), v.size());
                ImPlot::EndPlot();
            }

            i++;
        }
        ImGui::EndTable();
    }
}

}  // namespace imgui
}  // namespace roboflex