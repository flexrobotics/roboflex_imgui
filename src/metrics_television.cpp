#include "roboflex_imgui/metrics_television.h"
#include "roboflex_core/core_nodes/metrics.h"
#include <implot.h>

namespace roboflex {
namespace imgui {


// --- MetricSeries ---

void MetricSeries::append(const roboflex::nodes::MetricTracker& mt, const std::string& name, double t, double elapsed_time, double tnow) {

    if (mt.count == 0) {
        return;
    }

    if (name == "frequency") {
        double meanv = mt.mean_value == 0 ? 0 : 1.0 / mt.mean_value;
        double minv = mt.max_value == 0 ? 0 : 1.0 / mt.max_value;
        double maxv = mt.mean_value == 0 ? 0 : 2.0 / mt.mean_value;
        means.push_back(meanv);
        mins.push_back(minv);
        maxes.push_back(maxv);
    } else if (name == "time fraction") {
        means.push_back(mt.count * mt.mean_value); // divided by elapsed time?
        mins.push_back(mt.count * mt.min_value);
        maxes.push_back(mt.count * mt.max_value);
    } else {
        means.push_back(mt.mean_value);
        maxes.push_back(mt.max_value);
        mins.push_back(mt.min_value);
    }
    times.push_back(t);
    totals.push_back(mt.total / elapsed_time);

    while (times.front() < (tnow-60.0)) {
        means.erase(means.begin());
        maxes.erase(maxes.begin());
        mins.erase(mins.begin());
        times.erase(times.begin());
        totals.erase(totals.begin());
    }
}

void display_metric_series(const MetricSeries& ms, const std::string& name, double tnow)
{
    if (ms.times.size() > 0) {

        //auto flags = ImPlotFlags_CanvasOnly | ImPlotFlags_NoHighlight | ImPlotFlags_NoChild;
        //auto flags = ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMousePos | ImPlotFlags_NoHighlight | ImPlotFlags_NoChild;
        auto flags = ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText | ImPlotAxisFlags_NoHighlight | ImPlotFlags_NoChild;
        auto x_flags = ImPlotAxisFlags_NoDecorations;
        auto y_flags = ImPlotAxisFlags_NoGridLines;

        auto color_1 = ImVec4(1, 0, 0, 1);
        auto color_2 = ImVec4(0, 1, 0, 1);
        //auto color_3 = ImVec4(0, 0, 1, 1);

        double ymin = 0;
        double ymax = 1.0;
        if (name == "time fraction") {
            //double ymax_max = *std::max_element(ms.maxes.begin(), ms.maxes.end());
            double ymax_mean = *std::max_element(ms.means.begin(), ms.means.end());
            ymax = ymax_mean; // ceil(std::max(ymax_max, ymax_mean));
        } else if (name == "bytes") {
            ymax = *std::max_element(ms.totals.begin(), ms.totals.end());
        } else {
            //double ymax_max = *std::max_element(ms.maxes.begin(), ms.maxes.end());
            double ymax_mean = *std::max_element(ms.means.begin(), ms.means.end());
            ymax = ymax_mean; // std::max(ymax_max, ymax_mean);
        }

        auto &v = name == "bytes" ? ms.totals : ms.means;

        ymax = std::max(0.01, ymax * 1.4);
        //ImPlot::SetNextPlotLimits(tnow-60, tnow+8, ymin, ymax, ImGuiCond_Always);
        ImPlot::SetNextAxesLimits(tnow-60, tnow+8, ymin, ymax, ImGuiCond_Always);

        //auto marker = name == "missed" ? ImPlotMarker_Circle : ImPlotMarker_None;
        bool should_shade = name != "missed";

        double limit = name == "missed" ? 0 : name == "time fraction" ? 0.5 : name == "latency" ? 0.1 : std::numeric_limits<double>::max();

        bool alert = std::any_of(v.begin(), v.end(), [limit](double k){return k>limit;});
        auto color = alert ? color_1 : color_2;

        auto last = v.size() > 0 ? v[v.size()-1] : 0;
        std::stringstream s;
        s.imbue(std::locale(""));
        s << std::fixed << std::setprecision(3) << last;
        const std::string last_string = s.str();

        //ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(30,0));
        if (ImPlot::BeginPlot(last_string.c_str(), ImVec2(-1,100), flags)) {

            ImPlot::SetupAxis(ImAxis_X1, NULL, x_flags);
            ImPlot::SetupAxis(ImAxis_Y1, NULL, y_flags);

            ImPlot::PushStyleColor(ImPlotCol_Fill, color);
            ImPlot::PushStyleColor(ImPlotCol_Line, color);
            if (should_shade) {
                ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.2f);
                ImPlot::PlotShaded("##noid", ms.times.data(), v.data(), ms.size());
                ImPlot::PopStyleVar();
            }
            ImPlot::PushStyleVar(ImPlotStyleVar_LineWeight, 1);
            ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 1);
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);

            ImPlot::PlotLine("##noid", ms.times.data(), v.data(), ms.size());

            ImPlot::PopStyleVar();
            ImPlot::PopStyleVar();
            ImPlot::PopStyleColor();
            ImPlot::PopStyleColor();

            ImPlot::EndPlot();
        }
        //ImPlot::PopStyleVar();
    }
}

std::string get_link_name(const std::string& from, const std::string& to) {
    return from + "\n-> " + to;
}


// --- MetricsTelevision ---

MetricsTelevision::MetricsTelevision(
    const string& window_title,
    const pair<int, int>& initial_size,
    const pair<int, int>& initial_pos,
    const string& name,
    const bool debug):
        IMPLOTNode(window_title, initial_size, initial_pos, name, debug)
{

}

void MetricsTelevision::receive(roboflex::core::MessagePtr m)
{
    double t = roboflex::core::get_current_time();

    auto mm = roboflex::nodes::MetricsMessage(*m);
    double mt = mm.timestamp();
    double et = mm.elapsed_time();

    const std::lock_guard<std::mutex> lock(mtx);

    GuidToGuid guid_to_guid = GuidToGuid(
        mm.parent_node_guid().str(),
        mm.child_node_guid().str());

    if (links_to_metrics.count(guid_to_guid) == 0) {
        links_to_metrics[guid_to_guid] = MetricsRow(
            mm.parent_node_name(),
            mm.child_node_name(),
            mm.host_name(),
            MetricSeriesCollection());
    }
    MetricsRow& mr = links_to_metrics[guid_to_guid];
    MetricSeriesCollection& v = std::get<3>(mr);

    v["frequency"].append(mm.metrics["dt"], "frequency", mt, et, t);
    v["latency"].append(mm.metrics["latency"], "latency", mt, et, t);
    v["time"].append(mm.metrics["time"], "time", mt, et, t);
    v["time fraction"].append(mm.metrics["time"], "time fraction", mt, et, t);
    v["bytes"].append(mm.metrics["bytes"], "bytes", mt, et, t);
    v["missed"].append(mm.metrics["missed"], "missed", mt, et, t);
}

void MetricsTelevision::draw() {

    const int even_background_color = 0x00000000;
    const int odd_background_color = 0x55555555;

    double tnow = roboflex::core::get_current_time();

    const std::lock_guard<std::mutex> lock(mtx);

    if (ImGui::Button("Clear")) {
        links_to_metrics.clear();
    }

    if (ImGui::BeginTable("table", 7, 0, ImVec2(-1,0))) {

        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Frequency, Hz");
        ImGui::TableSetupColumn("Latency, seconds");
        ImGui::TableSetupColumn("Time in receive, seconds");
        ImGui::TableSetupColumn("CPU Core Fractional Usage");
        ImGui::TableSetupColumn("Bytes/second");
        ImGui::TableSetupColumn("Missed Messages");
        ImGui::TableHeadersRow();

        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, odd_background_color, 1);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, even_background_color, 2);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, odd_background_color, 3);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, even_background_color, 4);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, odd_background_color, 5);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, even_background_color, 6);

        std::vector<GuidToGuid> sorted_keys = sorted_metrics(this->links_to_metrics);

        int row = 0;
        for (auto guid_to_guid : sorted_keys) {

            const std::string guid_1 = guid_to_guid.first;
            const std::string guid_2 = guid_to_guid.second;

            if (guid_1.size() == 0) {
                ImGui::TableNextRow(0, 24.0);
            } else {

                MetricsRow& metrics_row = this->links_to_metrics[guid_to_guid];

                const std::string& from_node_name = std::get<0>(metrics_row);
                const std::string& to_node_name = std::get<1>(metrics_row);
                const std::string& host_name = std::get<2>(metrics_row);
                const MetricSeriesCollection& d = std::get<3>(metrics_row);

                const std::string name = get_link_name(from_node_name, to_node_name) + "\n(on " + host_name + ")";

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                ImGui::Text("%s", name.c_str());
                ImGui::PushID(row);

                ImGui::TableSetColumnIndex(1);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, odd_background_color);
                display_metric_series(d.at("frequency"), "frequency", tnow);
                ImGui::TableSetColumnIndex(2);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, even_background_color);
                display_metric_series(d.at("latency"), "latency", tnow);
                ImGui::TableSetColumnIndex(3);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, odd_background_color);
                display_metric_series(d.at("time"), "time", tnow);
                ImGui::TableSetColumnIndex(4);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, even_background_color);
                display_metric_series(d.at("time fraction"), "time fraction", tnow);
                ImGui::TableSetColumnIndex(5);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, odd_background_color);
                display_metric_series(d.at("bytes"), "bytes", tnow);
                ImGui::TableSetColumnIndex(6);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, even_background_color);
                display_metric_series(d.at("missed"), "missed", tnow);

                ImGui::PopID();
            }

            row++;
        }

        ImGui::EndTable();
    }
}


MetricsTelevision::AdjacencyMap MetricsTelevision::compute_adjacency_map(const LinksToMetrics& metrics) {
    MetricsTelevision::AdjacencyMap l;
    for (auto [guid_to_guid, metrics_row]: metrics) {
        auto [from_guid, to_guid] = guid_to_guid;
        auto [from_node_name, to_node_name, host_name, metrics_collection] = metrics_row;

        AdjacencyItem& ai = l[from_guid];
        Adjacencies& ac = ai.second;
        ac.push_back(to_guid);
    }
    return l;
}

void MetricsTelevision::topological_sort_helper(
    const NodeGuid& from_guid,
    const AdjacencyMap& adjacency_map,
    const LinksToMetrics& l2m,
    std::vector<GuidToGuid>& into,
    std::set<NodeGuid>& visited)
{
    if (visited.find(from_guid) != visited.end()) {
        return;
    }

    visited.insert(from_guid);

    if (adjacency_map.count(from_guid) > 0) {
        const AdjacencyItem& adjacency_item = adjacency_map.at(from_guid);
        const Adjacencies& adjacencies = adjacency_item.second;

        // sort by row title
        std::vector<NodeGuid> to_guids_sorted_by_node_name(adjacencies);
        std::sort(
            std::begin(to_guids_sorted_by_node_name),
            std::end(to_guids_sorted_by_node_name),
            [&from_guid, &l2m](NodeGuid a, NodeGuid b) {
                const GuidToGuid f2a(from_guid, a);
                const GuidToGuid f2b(from_guid, b);
                const MetricsRow& mra = l2m.at(f2a);
                const MetricsRow& mrb = l2m.at(f2b);
                const NodeName& an = std::get<1>(mra);
                const NodeName& bn = std::get<1>(mrb);
                return an < bn;
            }
        );

        for (auto to_guid: to_guids_sorted_by_node_name) {
            into.push_back(GuidToGuid(from_guid, to_guid));
            topological_sort_helper(to_guid, adjacency_map, l2m, into, visited);
        }
    }
}

std::vector<MetricsTelevision::GuidToGuid> MetricsTelevision::topogical_sort(const AdjacencyMap& adjacency_map)
{
    std::map<NodeGuid, int> indegree;

    // compute indegree for all nodes
    for (auto [node_guid, adjacency_item]: adjacency_map) {
        for (auto to_guid: adjacency_item.second) {
            indegree[to_guid]++;
        }
    }

    // enqueue all nodes with indegree 0
    std::vector<NodeGuid> root_nodes;
    for (auto [node_guid, adjacency_item]: adjacency_map) {
        if (indegree[node_guid] == 0) {
            root_nodes.push_back(node_guid);
        }
    }

    std::set<NodeGuid> visited;

    std::vector<GuidToGuid> sorted_keys;
    for (auto root_node_guid: root_nodes) {
        sorted_keys.push_back(GuidToGuid());
        topological_sort_helper(root_node_guid, adjacency_map, this->links_to_metrics, sorted_keys, visited);
    }

    return sorted_keys;
}

std::string MetricsTelevision::links_to_metrics_to_string(const LinksToMetrics& metrics)
{
    std::stringstream sst;
    for (auto [guid_to_guid, metrics_row]: metrics) {
        auto [from_guid, to_guid] = guid_to_guid;
        auto [from_node_name, to_node_name, host_name, metrics_collection] = metrics_row;
        sst << from_node_name << " -> " << to_node_name << std::endl;
    }
    return sst.str();
}

std::vector<MetricsTelevision::GuidToGuid> MetricsTelevision::sorted_metrics(const LinksToMetrics& metrics)
{
    AdjacencyMap adjacency_map = compute_adjacency_map(metrics);
    std::vector<GuidToGuid> ordered_metrics = topogical_sort(adjacency_map);
    return ordered_metrics;
}


}  // namespace imgui
}  // namespace roboflex