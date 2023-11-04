#include <pybind11/pybind11.h>
#include "roboflex_core/core.h"
#include "roboflex_imgui/implot_node.h"
#include "roboflex_imgui/oned_television.h"
#include "roboflex_imgui/metrics_television.h"

namespace py  = pybind11;
using namespace roboflex::imgui;

PYBIND11_MODULE(roboflex_imgui_ext, m) {

    py::class_<IMPLOTNode, roboflex::core::RunnableNode, std::shared_ptr<IMPLOTNode>>(m, "IMPLOTNode")
        .def(py::init<const std::string&, const pair<int, int>&, const pair<int, int>&, const std::string&, const bool>(),
            "Create an IMPLOTNode node.",
            py::arg("window_title") = "IMPLOTNode",
            py::arg("initial_size") = pair<int, int>{640, 220},
            py::arg("initial_pos") = pair<int, int>{-1, -1},
            py::arg("name") = "IMPLOTNode",
            py::arg("debug") = false)
    ;

    py::class_<OneDTV, IMPLOTNode, std::shared_ptr<OneDTV>>(m, "OneDTV")
        .def(py::init<const std::string&, const std::string&, const unsigned int, const bool, const pair<int, int>&, const pair<int, int>&, const std::string&, const bool>(),
            "Create a OneDTV node.",
            py::arg("window_title") = "OneDTV",
            py::arg("data_key") = "data",
            py::arg("sample_size") = 4,
            py::arg("center_zero") = true,
            py::arg("initial_size") = pair<int, int>{640, 220},
            py::arg("initial_pos") = pair<int, int>{-1, -1},
            py::arg("name") = "OneDTV",
            py::arg("debug") = false)
    ;

    py::class_<MetricsTelevision, IMPLOTNode, std::shared_ptr<MetricsTelevision>>(m, "MetricsTelevision")
        .def(py::init<const std::string&, const pair<int, int>&, const pair<int, int>&, const std::string&, const bool>(),
            "Create a MetricsTelevision node.",
            py::arg("window_title") = "MetricsTelevision",
            py::arg("initial_size") = pair<int, int>{1580, 720},
            py::arg("initial_pos") = pair<int, int>{-1, -1},
            py::arg("name") = "MetricsTV",
            py::arg("debug") = false)
    ;
}
