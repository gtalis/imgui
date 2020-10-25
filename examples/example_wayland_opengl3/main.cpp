// Dear ImGui: standalone example application for Wayland + OpenGLES 2, using programmable pipeline
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <assert.h>

#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <string.h>
#include <linux/input.h>

#define WIDTH   1280
#define HEIGHT  720

static struct wl_display *display;
static struct wl_compositor *compositor = NULL;
static struct wl_shell *shell = NULL;
static struct wl_seat *seat = NULL;
static struct wl_pointer *pointer = NULL;

static int    pointer_x = 0;
static int    pointer_y = 0;

static int    pointer_btn_down[3] = { 0 };

static EGLDisplay egl_display;
static char running = 1;

struct window {
    EGLContext egl_context;
    struct wl_surface *surface;
    struct wl_shell_surface *shell_surface;
    struct wl_egl_window *egl_window;
    EGLSurface egl_surface;
};

static void
pointer_handle_enter(void *data, struct wl_pointer *pointer,
             uint32_t serial, struct wl_surface *surface,
             wl_fixed_t sx, wl_fixed_t sy)
{
}

static void
pointer_handle_leave(void *data, struct wl_pointer *pointer,
             uint32_t serial, struct wl_surface *surface)
{
}

static void
pointer_handle_motion(void *data, struct wl_pointer *pointer,
              uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    // mutex needed here, but this is just a quick and dirty test
    pointer_x = wl_fixed_to_int(sx);
    pointer_y = wl_fixed_to_int(sy);    
}

static void
pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
              uint32_t serial, uint32_t time, uint32_t button,
              uint32_t state)
{
     // mutex needed here, but this is just a quick and dirty test   
    int index = button - BTN_LEFT;
    pointer_btn_down[index] = (int) state;
}

static void
pointer_handle_axis(void *data, struct wl_pointer *wl_pointer,
            uint32_t time, uint32_t axis, wl_fixed_t value)
{
}

static const struct wl_pointer_listener pointer_listener = {
    pointer_handle_enter,
    pointer_handle_leave,
    pointer_handle_motion,
    pointer_handle_button,
    pointer_handle_axis,
};

static void
seat_handle_capabilities(void *data, struct wl_seat *seat,
             uint32_t caps)
{
    if (caps & WL_SEAT_CAPABILITY_POINTER) {
        pointer = (struct wl_pointer *) wl_seat_get_pointer(seat);
        wl_pointer_add_listener(pointer, &pointer_listener, NULL);
    }
}

static const struct wl_seat_listener seat_listener = {
    seat_handle_capabilities,
};

// listeners
static void registry_add_object (void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
    if (!strcmp(interface,"wl_compositor")) {
        compositor = (struct wl_compositor *) wl_registry_bind (registry, name, &wl_compositor_interface, 1);
    }
    else if (!strcmp(interface,"wl_shell")) {
        shell = (struct wl_shell *) wl_registry_bind (registry, name, &wl_shell_interface, 1);
    }
    else if (!strcmp(interface,"wl_seat")) {
        seat = (struct wl_seat *) wl_registry_bind (registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(seat, &seat_listener, NULL);
    }
}

static void registry_remove_object (void *data, struct wl_registry *registry, uint32_t name) {
    
}
static struct wl_registry_listener registry_listener = {&registry_add_object, &registry_remove_object};

static void shell_surface_ping (void *data, struct wl_shell_surface *shell_surface, uint32_t serial) {
    wl_shell_surface_pong (shell_surface, serial);
}
static void shell_surface_configure (void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height) {
    struct window *window = (struct window *)data;
    wl_egl_window_resize (window->egl_window, width, height, 0, 0);
}
static void shell_surface_popup_done (void *data, struct wl_shell_surface *shell_surface) {
    
}
static struct wl_shell_surface_listener shell_surface_listener = {&shell_surface_ping, &shell_surface_configure, &shell_surface_popup_done};

static void create_window (struct window *window, int32_t width, int32_t height) {
    eglBindAPI (EGL_OPENGL_ES_API);

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    const char *extensions;

    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 1,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig config;
    EGLint num_config;
    eglChooseConfig (egl_display, config_attribs, &config, 1, &num_config);
    window->egl_context = eglCreateContext (egl_display, config, EGL_NO_CONTEXT, context_attribs);
    
    window->surface = wl_compositor_create_surface (compositor);
    window->shell_surface = wl_shell_get_shell_surface (shell, window->surface);
    wl_shell_surface_add_listener (window->shell_surface, &shell_surface_listener, window);
    wl_shell_surface_set_toplevel (window->shell_surface);
    window->egl_window = wl_egl_window_create (window->surface, width, height);
    window->egl_surface = eglCreateWindowSurface (egl_display, config, window->egl_window, NULL);
    eglMakeCurrent (egl_display, window->egl_surface, window->egl_surface, window->egl_context);
}

static void delete_window (struct window *window) {
    eglDestroySurface (egl_display, window->egl_surface);
    wl_egl_window_destroy (window->egl_window);
    wl_shell_surface_destroy (window->shell_surface);
    wl_surface_destroy (window->surface);
    eglDestroyContext (egl_display, window->egl_context);
}

static void draw_window (struct window *window) {
    glClearColor (0.0, 1.0, 0.0, 1.0);
    glClear (GL_COLOR_BUFFER_BIT);
    eglSwapBuffers (egl_display, window->egl_surface);
}

static void ImGui_ImplWayland_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer backend. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    io.DisplaySize = ImVec2((float)WIDTH, (float)HEIGHT);
    io.DisplayFramebufferScale = ImVec2((float)1, (float)1);

    io.DeltaTime = (float)(1.0f / 60.0f);

    // mutex needed here, but this is just a quick and dirty test
    io.MousePos = ImVec2(pointer_x, pointer_y);
    for (int i = 0; i < 3; i++) {
        io.MouseDown[i] = pointer_btn_down[i];
    }
}

int main(int, char**)
{
    display = wl_display_connect (NULL);
    struct wl_registry *registry = wl_display_get_registry (display);
    wl_registry_add_listener (registry, &registry_listener, NULL);
    wl_display_roundtrip (display);
    
    egl_display = eglGetDisplay (display);
    eglInitialize (egl_display, NULL, NULL);
    
    struct window window;
    create_window (&window, WIDTH, HEIGHT);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplOpenGL3_Init(NULL /* GLES2 */);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (running)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        wl_display_dispatch_pending(display);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWayland_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        display_w = WIDTH;
        display_h = HEIGHT;
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        eglSwapBuffers(egl_display, window.egl_surface);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();

    delete_window(&window);

    if (compositor)
        wl_compositor_destroy(compositor);

    wl_registry_destroy(registry);
    wl_display_flush(display);
    wl_display_disconnect(display);

    return 0;
}