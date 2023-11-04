#ifndef ROBOFLEX_METRICS_TELEVISION__H
#define ROBOFLEX_METRICS_TELEVISION__H

#include "roboflex_core/core.h"
#include "implot_node.h"
#include "roboflex_core/core_nodes/core_nodes.h"

namespace roboflex {
namespace imgui {

using std::string, std::pair, core::MessagePtr;

struct MetricSeries {
    // A single metric time series.

    std::vector<double> means;
    std::vector<double> maxes;
    std::vector<double> mins;
    std::vector<double> totals;
    std::vector<double> times;

    size_t size() const { return times.size(); }

    void append(const roboflex::nodes::MetricTracker& mt, const std::string& name, double t, double elapsed_time, double tnow);
};

class MetricsTelevision: public IMPLOTNode {
public:
    MetricsTelevision(
        const string& window_title = "MetricsTelevision",
        const pair<int, int>& initial_size = {1580, 720},
        const pair<int, int>& initial_pos = {-1, -1},
        const string& name = "MetricsTelevision",
        const bool debug = false);

protected:

    void draw() override;
    void receive(MessagePtr m) override;

    mutable std::mutex mtx;

    using NodeGuid = std::string;
    using NodeName = std::string;
    using HostName = std::string;
    using GuidToGuid = std::pair<NodeGuid, NodeGuid>;
    using ColTitle = std::string;
    using MetricSeriesCollection = std::map<ColTitle, MetricSeries>;
    using MetricsRow = std::tuple<NodeName, NodeName, HostName, MetricSeriesCollection>;
    using LinksToMetrics = std::map<GuidToGuid, MetricsRow>;

    LinksToMetrics links_to_metrics;

    using Adjacencies = std::vector<NodeGuid>;
    using AdjacencyItem = std::pair<MetricsRow, Adjacencies>;
    using AdjacencyMap = std::map<NodeGuid, AdjacencyItem>;

    AdjacencyMap compute_adjacency_map(const LinksToMetrics& metrics);
    void topological_sort_helper(
        const NodeGuid& from_guid,
        const AdjacencyMap& adjacency_map,
        const LinksToMetrics& l2m,
        std::vector<GuidToGuid>& into,
        std::set<NodeGuid>& visited);
    std::vector<GuidToGuid> topogical_sort(const AdjacencyMap& adjacency_map);
    std::string links_to_metrics_to_string(const LinksToMetrics& metrics);
    std::vector<GuidToGuid> sorted_metrics(const LinksToMetrics& metrics);
};

}  // namespace imgui
}  // namespace roboflex

#endif // ROBOFLEX_METRICS_TELEVISION__H
