#include <algorithm>
#include "roboflex_imgui/imgui_nodes.h"
#include <GL/glew.h>
#include <implot.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>
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


// -- IMPLOTNode --

IMPLOTNode::IMPLOTNode(
    const pair<int, int>& initial_size,
    const pair<int, int>& initial_pos,
    const string& name,
    const bool debug):
        core::RunnableNode(name),
        initial_size(initial_size),
        initial_pos(initial_pos),
        debug(debug)
{

}

void IMPLOTNode::child_thread_fn()
{
    uint64_t invokations = 0;

    // BOILERPLATE SDL INITIALIZATION
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        auto err = std::string("IMPLOTNode Error calling SDL_Init: ") + SDL_GetError();
        throw std::runtime_error(err);
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_Window* window  = SDL_CreateWindow(name.c_str(), 
        initial_pos.first, initial_pos.second, 
        initial_size.first, initial_size.second, 
        window_flags);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    bool err = glewInit() != GLEW_OK;
    if (err) {
        auto err = std::string("IMPLOTNode Error calling glewInit");
        throw std::runtime_error(err);
    }

    IMGUI_CHECKVERSION();

    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    double t0 = core::get_current_time();

    while (!this->stop_requested()) {

        // POLL FOR EVENTS
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT ||
                (event.type == SDL_WINDOWEVENT && 
                 event.window.event == SDL_WINDOWEVENT_CLOSE && 
                 event.window.windowID == SDL_GetWindowID(window)))
            {
                this->request_stop();
            }
        }

        invokations++;

        // BOILERPLATE GETTING READY TO DRAW
        ImGuiIO& io = ImGui::GetIO();

        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        // set up imgui window and get ready to draw
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();
        //ImGui::ShowDemoWindow();
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration; // | ImGuiWindowFlags_NoSavedSettings;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        bool p_open;
        ImGui::Begin(get_name().c_str(), &p_open, flags);


        // OUR CUSTOM DRAWING
        this->draw();


        // MORE BOILERPLATE...

        ImGui::End();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    double t1 = core::get_current_time();
    if (debug) {
        std::cerr << "IMGUINode ran for " << t1 - t0 << " seconds, " << invokations << " draw calls, frequency=" << (invokations / (t1-t0)) << std::endl;
    }
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}


// -- OneDTV --

OneDTV::OneDTV(
    const string& data_key,
    const unsigned int sample_size,
    const bool center_zero,
    const pair<int, int>& initial_size,
    const pair<int, int>& initial_pos,
    const string& name,
    const bool debug):
        IMPLOTNode(initial_size, initial_pos, name, debug),
        data_key(data_key),
        sample_size(sample_size),
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

        int i = 0;
        for (const auto v: values) {

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
            if (ImPlot::BeginPlot("", ImVec2(-1,100), flags)) {
                ImPlot::SetupAxis(ImAxis_X1, NULL, x_flags);
                ImPlot::SetupAxis(ImAxis_Y1, NULL, y_flags);
                ImPlot::PlotLine("##noid", v.data(), v.size());
                ImPlot::EndPlot();
            }

            i++;
        }
        ImGui::EndTable();
    }
}

}  // namespace imgui
}  // namespace roboflex