#include "graphics.h"

#include <iostream>
#include <chrono>
#include <format>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <locale>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using namespace std;

static GraphicsEnv* graphics_env = nullptr;

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    glfwSwapBuffers(window);
    glViewport(0, 0, width, height);
}

static void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_REPEAT)
        return;

    switch (key)
    {
    case GLFW_KEY_W: graphics_env->camera_velocity.z += (action == GLFW_PRESS) ? -1.0f : 1.0f; break;
    case GLFW_KEY_S: graphics_env->camera_velocity.z += (action == GLFW_PRESS) ? 1.0f : -1.0f; break;
    case GLFW_KEY_D: graphics_env->camera_velocity.x += (action == GLFW_PRESS) ? 1.0f : -1.0f; break;
    case GLFW_KEY_A: graphics_env->camera_velocity.x += (action == GLFW_PRESS) ? -1.0f : 1.0f; break;
    case GLFW_KEY_E: graphics_env->camera_velocity.y += (action == GLFW_PRESS) ? 1.0f : -1.0f; break;
    case GLFW_KEY_Q: graphics_env->camera_velocity.y += (action == GLFW_PRESS) ? -1.0f : 1.0f; break;
    }
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_2)
    {
        if (action == GLFW_PRESS)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else if (action == GLFW_RELEASE)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

static void mouseMovedCallback(GLFWwindow* window, double xpos, double ypos)
{
    static double old_xpos = xpos;
    static double old_ypos = ypos;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
    {
        graphics_env->camera_euler.z += (old_xpos - xpos) * 0.12f;
        graphics_env->camera_euler.x += (old_ypos - ypos) * 0.12f;
    }
    old_xpos = xpos;
    old_ypos = ypos;
}

static const char* vertex_shader_source = R"(
#version 330 core

layout (location = 0) in vec3 vertex_position;

uniform mat4 world_to_clip;

varying vec3 position;

void main()
{
    position = vertex_position;
    gl_Position = world_to_clip * vec4(vertex_position, 1.0f);
}
)";

static const char* fragment_shader_source = R"(
#version 330 core

out vec4 FragColor;

uniform float is_inverted;

varying vec3 position;

void main()
{
    FragColor = (is_inverted > 0.5f) ? vec4(1.0f, 0.0f, 0.0f, 1.0f) : vec4(position, 1.0f);
}
)";

bool GraphicsEnv::create(int width, int height)
{
    // initialise glfw window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(width, height, "fuck", nullptr, nullptr);
    if (!window)
    {
        cout << "could not create window" << endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "failed to initialize GLAD" << endl;
        return false;
    }
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyboardCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mouseMovedCallback);
    glViewport(0, 0, width, height);
    configureImGui();

    // create vertex and index buffers
    glGenBuffers(1, &vertex_buffer);
    glGenBuffers(1, &index_buffer);

    // create shaders
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
    glCompileShader(vertex_shader);
    int status;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        string error; error.resize(512);
        glGetShaderInfoLog(vertex_shader, error.size(), nullptr, error.data());
        cout << "vertex shader error: " << error << endl;
        return false;
    }

    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        string error; error.resize(512);
        glGetShaderInfoLog(fragment_shader, error.size(), nullptr, error.data());
        cout << "fragment shader error: " << error << endl;
        return false;
    }

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &status);
    if (!status)
    {
        string error; error.resize(512);
        glGetProgramInfoLog(shader_program, error.size(), NULL, error.data());
        cout << "shader program error: " << error << endl;
        return false;
    }
    // create uniforms
    shu_transform_location = glGetUniformLocation(shader_program, "world_to_clip");
    shu_is_inverted_location = glGetUniformLocation(shader_program, "is_inverted");
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // specify vertex data format
    glGenVertexArrays(1, &vertex_array_object);
    glBindVertexArray(vertex_array_object);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MTVT::Vector3), (void*)0);
    glEnableVertexAttribArray(0);

    // specify clear values
    glClearColor(0.004f, 0.509f, 0.506f, 1.0f);
    glClearDepth(1.0f);

    // initialise depth buffer
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);

    graphics_env = this;

    return true;
}

void GraphicsEnv::setMesh(MTVT::Mesh mesh)
{
    mesh_data = mesh;
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(MTVT::Vector3) * mesh_data.vertices.size(), mesh_data.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(MTVT::VertexRef) * mesh_data.indices.size(), mesh_data.indices.data(), GL_STATIC_DRAW);
}

void GraphicsEnv::setSummary(MTVT::SummaryStats summary)
{
    summary_stats = summary;
}

bool GraphicsEnv::draw()
{
    if (glfwWindowShouldClose(window))
        return false;
    glfwPollEvents();

    auto now = chrono::steady_clock::now();
    chrono::duration<float> time_since_last_frame = now - last_frame_time;
    last_frame_time = now;

    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::rotate(transform, -glm::radians(camera_euler.x), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, -glm::radians(camera_euler.y), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, -glm::radians(camera_euler.z), glm::vec3(0.0f, 0.0f, 1.0f));

    float delta_time = time_since_last_frame.count();
    camera_position += (glm::vec4(camera_velocity, 0.0f) * transform) * delta_time;

    int width, height; glfwGetFramebufferSize(window, &width, &height);
    transform = glm::translate(transform, -camera_position);
    transform = glm::perspective(90.0f, (float)width / (float)height, 0.0001f, 100.0f) * transform;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shader_program);
    glBindVertexArray(vertex_array_object);
    glUniformMatrix4fv(shu_transform_location, 1, GL_FALSE, glm::value_ptr(transform));

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glUniform1f(shu_is_inverted_location, 0.0f);
    glDrawElements(GL_TRIANGLES, mesh_data.indices.size(), GL_UNSIGNED_INT, 0);
    glCullFace(GL_FRONT);
    glUniform1f(shu_is_inverted_location, 1.0f);
    glDrawElements(GL_TRIANGLES, mesh_data.indices.size(), GL_UNSIGNED_INT, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_CULL_FACE);
    glDrawElements(GL_TRIANGLES, mesh_data.indices.size(), GL_UNSIGNED_INT, 0);

    drawImGui();
    // TODO: draw imgui
    // TODO: act based on imgui

    glfwSwapInterval(1);
    glfwSwapBuffers(window);
    return true;
}

void GraphicsEnv::destroy()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

void GraphicsEnv::configureImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
}

void GraphicsEnv::drawImGui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("##somename");
        ImGui::Text("%f fps", ImGui::GetIO().Framerate);
        ImGui::Text("%f ms", ImGui::GetIO().DeltaTime * 1000.0f);
        ImGui::End();
    }
    {
        ImGui::Begin("mesh info", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        string vertices_label = format(locale("en_US.UTF-8"),  "vertices  {0:>12L} ({1})", summary_stats.vertices, MTVT::getMemorySize(summary_stats.vertices_bytes));
        ImGui::Text(vertices_label.c_str());
        string triangles_label = format(locale("en_US.UTF-8"), "triangles {0:>12L} ({1})", summary_stats.triangles, MTVT::getMemorySize(summary_stats.indices_bytes));
        ImGui::Text(triangles_label.c_str());
        ImGui::End();
    }
    {
        ImGui::Begin("generation stats", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::LabelText("resolution", "%i x %i x %i", summary_stats.cubes_x, summary_stats.cubes_y, summary_stats.cubes_z);
        ImGui::LabelText("lattice type", summary_stats.lattice_type.c_str());
        ImGui::LabelText("clustering mode", summary_stats.clustering_mode.c_str());
        ImGui::Separator();
        ImGui::LabelText("sample points", format(locale("en_US.UTF-8"), "{0:L} ({1:L} ideal, {2})", summary_stats.sample_points_allocated, summary_stats.sample_points_theoretical, MTVT::getMemorySize(summary_stats.sample_points_bytes)).c_str());
        ImGui::LabelText("edges", format(locale("en_US.UTF-8"), "{0:L} ({1:L} ideal, {2})", summary_stats.edges_allocated, summary_stats.edges_theoretical, MTVT::getMemorySize(summary_stats.edges_bytes)).c_str());
        ImGui::LabelText("tetrahedra", format(locale("en_US.UTF-8"), "{0:L} ({1:L} total, {2:.2f}%%)", summary_stats.tetrahedra_computed, summary_stats.tetrahedra_total, summary_stats.tetrahedra_computed_percent).c_str());
        ImGui::End();
    }
    {
        ImGui::Begin("timing stats", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::LabelText("total", "%.8fs", summary_stats.time_total);
        ImGui::Separator();
        ImGui::LabelText("allocation", "%.8fs (%.2f%%)", summary_stats.time_allocation, summary_stats.percent_allocation);
        ImGui::LabelText("sampling", "%.8fs (%.2f%%)", summary_stats.time_sampling, summary_stats.percent_sampling);
        ImGui::LabelText("vertex", "%.8fs (%.2f%%)", summary_stats.time_vertex, summary_stats.percent_vertex);
        ImGui::LabelText("geometry", "%.8fs (%.2f%%)", summary_stats.time_geometry, summary_stats.percent_geometry);
        ImGui::End();
    }
    {
        ImGui::Begin("geometry stats", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::LabelText("degenerate tris", "%d (%.3f%% of total)", summary_stats.degenerate_triangles, summary_stats.degenerate_percent);
        ImGui::Separator();
        ImGui::LabelText("verts / sp", "%.8f", summary_stats.verts_per_sp);
        ImGui::LabelText("verts / edge", "%.8f", summary_stats.verts_per_edge);
        ImGui::LabelText("verts / tet", "%.8f", summary_stats.verts_per_tet);
        ImGui::LabelText("tris / sp", "%.8f", summary_stats.tris_per_sp);
        ImGui::LabelText("tris / edge", "%.8f", summary_stats.tris_per_edge);
        ImGui::LabelText("tris / tet", "%.8f", summary_stats.tris_per_tet);
        ImGui::Separator();
        ImGui::Text("triangle area");
        ImGui::BeginTable("triangle area", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        {
            ImGui::TableSetupColumn("mean");
            ImGui::TableSetupColumn("max");
            ImGui::TableSetupColumn("min");
            ImGui::TableSetupColumn("std. dev.");
            ImGui::TableHeadersRow();
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%4f", summary_stats.triangle_stats.area_mean);
            ImGui::TableNextColumn();
            ImGui::Text("%4f", summary_stats.triangle_stats.area_max);
            ImGui::TableNextColumn();
            ImGui::Text("%4f", summary_stats.triangle_stats.area_min);
            ImGui::TableNextColumn();
            ImGui::Text("%4f", summary_stats.triangle_stats.area_sd);
        }
        ImGui::EndTable();
        ImGui::Spacing();
        ImGui::Text("triangle aspect ratio");
        ImGui::BeginTable("triangle aspect", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        {
            ImGui::TableSetupColumn("mean");
            ImGui::TableSetupColumn("max");
            ImGui::TableSetupColumn("min");
            ImGui::TableSetupColumn("std. dev.");
            ImGui::TableHeadersRow();
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%4f", summary_stats.triangle_stats.aspect_mean);
            ImGui::TableNextColumn();
            ImGui::Text("%4f", summary_stats.triangle_stats.aspect_max);
            ImGui::TableNextColumn();
            ImGui::Text("%4f", summary_stats.triangle_stats.aspect_min);
            ImGui::TableNextColumn();
            ImGui::Text("%4f", summary_stats.triangle_stats.aspect_sd);
        }
        ImGui::EndTable();
        ImGui::End();
    }
    {
        ImGui::Begin("generation controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        // TODO: control the generator, and run it
        ImGui::End();
    }
    {
        ImGui::Begin("view controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        // TODO: control the camera, vsync, fov, view modes (backfaces, wireframe, etc), also move framerate here
        ImGui::End();
    }
    {
        ImGui::Begin("script controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        // TODO: batch and benchmarking config, including gif generator, csv export, etc
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
