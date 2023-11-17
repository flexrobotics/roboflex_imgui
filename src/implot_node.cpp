#include <algorithm>
#include "roboflex_imgui/implot_node.h"
#include <GL/glew.h>
#include <implot.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>
#include "roboflex_core/serialization/flex_tensor_format.h"

namespace roboflex {
namespace imgui {


// -- IMPLOTNode --

IMPLOTNode::IMPLOTNode(
    const string& window_title,
    const pair<int, int>& initial_size,
    const pair<int, int>& initial_pos,
    const string& name,
    const bool debug):
        core::RunnableNode(name),
        window_title(window_title),
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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window  = SDL_CreateWindow(
        window_title.c_str(), 
        initial_pos.first, initial_pos.second, 
        initial_size.first, initial_size.second, 
        window_flags);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    auto glew_res = GLEW_OK;
    #ifndef __APPLE__
        // for some reason we don't do it on mac
        glew_res = glewInit();
    #endif

    bool err = glew_res != GLEW_OK;
    if (err) {
        auto err = std::string("IMPLOTNode Error calling glewInit");
        throw std::runtime_error(err);
    }

    IMGUI_CHECKVERSION();

    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);

#if __APPLE__
    // this might be "4.10"
    const unsigned char * glsls = glGetString(GL_SHADING_LANGUAGE_VERSION);

    // now we have "410"
    const char glsl_new_string[] = {(char)glsls[0], (char)glsls[2], (char)glsls[3], 0};
    
    // now we have "#version 410"
    string n = string("#version ") + glsl_new_string;
    const char* glsl_version = n.data();

#else
    const char* glsl_version = "#version 130";
#endif

    if (debug) {
        std::cout << "GL_VERSION: "<< glGetString(GL_VERSION) << std::endl;
        std::cout << "GL_SHADING_LANGUAGE_VERSION: "<< glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
        std::cout << "glsl_version passed to ImGui_ImplOpenGL3_Init: \""<< glsl_version << "\"" << std::endl;
    }

    ImGui_ImplOpenGL3_Init(glsl_version);

    double t0 = core::get_current_time();

    bool done = false;
    while (!done && !this->stop_requested()) {

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
                done = true;
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

}  // namespace imgui
}  // namespace roboflex