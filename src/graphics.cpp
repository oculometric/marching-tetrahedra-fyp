#include "graphics.h"

#include <iostream>
#include <chrono>
#include <format>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <locale>
#include <stb_image.h>
#define GIF_FLIP_VERT
#include <gif.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "demo_functions.h"
#include "backface_image_raw.h"

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
        graphics_env->camera_euler.z += (float)(old_xpos - xpos) * 0.12f;
        graphics_env->camera_euler.x += (float)(old_ypos - ypos) * 0.12f;
    }
    old_xpos = xpos;
    old_ypos = ypos;
}

static const char* vertex_shader_source = R"(
#version 330 core

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec3 vertex_normal;

uniform mat4 world_to_clip;

varying vec3 position;
varying vec3 world_normal;
varying vec3 screen_normal;

void main()
{
    position = vertex_position;
    world_normal = vertex_normal;
    gl_Position = world_to_clip * vec4(vertex_position, 1.0f);
    screen_normal = normalize(world_to_clip * vec4(vertex_normal, 0.0f)).xyz;
}
)";

static const char* fragment_shader_source = R"(
#version 330 core

out vec4 FragColor;

uniform int shading_mode;
uniform int backface_highlight;
uniform sampler2D backface_image;
uniform vec3 shading_colour_a;
uniform vec3 shading_colour_b;

varying vec3 position;
varying vec3 world_normal;
varying vec3 screen_normal;

const vec3 light_vec = vec3(-0.6f, 0.2f, -0.8f);

float saturate(float f)
{
    return clamp(f, 0.0f, 1.0f);
}

void main()
{
    bool should_highlight = (backface_highlight == 1 && !gl_FrontFacing) || (backface_highlight == 2 && gl_FrontFacing);

    if (should_highlight)
    {
        vec2 coord = (gl_FragCoord.xy / vec2(textureSize(backface_image, 0).xy)) * vec2(1.0f, -1.0f);
        coord.y = mod(coord.y, 0.5f);
        if (backface_highlight == 1)
            coord.y += 0.5f;
        FragColor = texture(backface_image, coord);
    }
    else
    {
        switch (shading_mode)
        {
            case 0:
                FragColor = vec4(0.8f, 0.8f, 0.8f, 1.0f); break;
            case 1:
                float dot_prod = dot(world_normal, -light_vec);
                vec3 back_shading = mix(shading_colour_b, shading_colour_b * 0.1f, -dot_prod);
                vec3 front_shading = mix(shading_colour_a, vec3(1.0f, 0.95f, 0.8f), pow(dot_prod, 3.0f));
                FragColor = vec4(mix(back_shading, front_shading, saturate(dot_prod)), 1.0f); break;
            case 2:
                FragColor = vec4(position, 1.0f); break;
            case 3:
                FragColor = vec4(world_normal, 1.0f); break;
        }
    }
}
)";

bool GraphicsEnv::create(int width, int height)
{
    // initialise glfw window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(width, height, "MTVT visualiser", nullptr, nullptr);
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
    glGenBuffers(1, &flat_vertex_buffer);

    // create shaders
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
    glCompileShader(vertex_shader);
    int status;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        string error; error.resize(512);
        glGetShaderInfoLog(vertex_shader, static_cast<int>(error.size()), nullptr, error.data());
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
        glGetShaderInfoLog(fragment_shader, static_cast<int>(error.size()), nullptr, error.data());
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
        glGetProgramInfoLog(shader_program, static_cast<int>(error.size()), NULL, error.data());
        cout << "shader program error: " << error << endl;
        return false;
    }
    // create uniforms
    shvar_transform = glGetUniformLocation(shader_program, "world_to_clip");
    shvar_shading_mode = glGetUniformLocation(shader_program, "shading_mode");
    shvar_backface_highlight = glGetUniformLocation(shader_program, "backface_highlight");
    shvar_shading_colour_a = glGetUniformLocation(shader_program, "shading_colour_a");
    shvar_shading_colour_b = glGetUniformLocation(shader_program, "shading_colour_b");
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // specify vertex data format
    glGenVertexArrays(1, &vertex_array_object);
    glBindVertexArray(vertex_array_object);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(Vertex), (void*)(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);

    // a copy of the vertex data but for the flat shaded version
    glGenVertexArrays(1, &flat_vertex_array_object);
    glBindVertexArray(flat_vertex_array_object);
    glBindBuffer(GL_ARRAY_BUFFER, flat_vertex_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(Vertex), (void*)(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);

    // specify clear values
    glClearColor(0.004f, 0.509f, 0.506f, 1.0f);
    glClearDepth(1.0f);

    // initialise depth buffer
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);

    // load image
    int img_width, img_height, channels;
    float* data = stbi_loadf_from_memory(backface_image_data, backface_image_data_size, &img_width, &img_height, &channels, 3);
    if (!data)
    {
        cout << "unable to load backface image!!" << endl;
        exit(-1);
    }
    glGenTextures(1, &backface_image);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, backface_image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_width, img_height, 0, GL_RGB, GL_FLOAT, data);
    stbi_image_free(data);

    graphics_env = this;

    return true;
}

void GraphicsEnv::setMesh(MTVT::Mesh mesh, MTVT::Vector3 offset)
{
    mesh_data = mesh;

    if (mesh.indices.empty())
        return;

    // reformat mesh data
    rearranged_vertex_data.clear();
    rearranged_vertex_data.resize(mesh_data.vertices.size());
    for (size_t i = 0; i < mesh_data.vertices.size(); ++i)
        rearranged_vertex_data[i] = { mesh_data.vertices[i] - offset, mesh_data.normals[i] };

    flat_shaded_data.clear();
    flat_shaded_data.reserve(mesh_data.indices.size());
    for (size_t i = 0; i < mesh_data.indices.size() - 2; i += 3)
    {
        const MTVT::VertexRef i0 = mesh_data.indices[i];
        const MTVT::VertexRef i1 = mesh_data.indices[i + 1];
        const MTVT::VertexRef i2 = mesh_data.indices[i + 2];

        const MTVT::Vector3 v0 = mesh_data.vertices[i0];
        const MTVT::Vector3 v1 = mesh_data.vertices[i1];
        const MTVT::Vector3 v2 = mesh_data.vertices[i2];

        MTVT::Vector3 normal = norm((v1 - v0) % (v2 - v0));
        flat_shaded_data.push_back({ v0 - offset, normal });
        flat_shaded_data.push_back({ v1 - offset, normal });
        flat_shaded_data.push_back({ v2 - offset, normal });
    }

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * rearranged_vertex_data.size(), rearranged_vertex_data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(MTVT::VertexRef) * mesh_data.indices.size(), mesh_data.indices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, flat_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * flat_shaded_data.size(), flat_shaded_data.data(), GL_STATIC_DRAW);
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

    transform = glm::translate(transform, -camera_position);

    int width, height; glfwGetFramebufferSize(window, &width, &height);
    float aspect = (float)width / (float)height;
    if (is_orthographic)
        transform = glm::ortho(-1.0f * aspect, 1.0f * aspect, -1.0f, 1.0f, 0.0001f, 100.0f) * transform;
    else
        transform = glm::perspective(camera_fov, aspect, 0.0001f, 100.0f) * transform;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!mesh_data.indices.empty())
        drawMesh(transform);
    drawImGui();

    glfwSwapInterval(vsync_enabled ? 1 : 0);
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

void GraphicsEnv::drawMesh(glm::mat4 transform)
{
    glUseProgram(shader_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, backface_image);
    if (smooth_shading)
        glBindVertexArray(vertex_array_object);
    else
        glBindVertexArray(flat_vertex_array_object);
    glUniformMatrix4fv(shvar_transform, 1, GL_FALSE, glm::value_ptr(transform));
    glUniform3f(shvar_shading_colour_a, shading_colour_a.r, shading_colour_a.g, shading_colour_a.b);
    glUniform3f(shvar_shading_colour_b, shading_colour_b.r, shading_colour_b.g, shading_colour_b.b);

    if (!wireframe_only)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        if (backface_mode == 0 || backface_mode > 2)
            glDisable(GL_CULL_FACE);
        else
        {
            glEnable(GL_CULL_FACE);
            glCullFace(backface_mode == 1 ? GL_BACK : GL_FRONT);
        }
        glUniform1i(shvar_shading_mode, shading_mode);
        glUniform1i(shvar_backface_highlight, (backface_mode <= 2) ? 0 : (backface_mode - 2));
        if (smooth_shading)
            glDrawElements(GL_TRIANGLES, static_cast<int>(mesh_data.indices.size()), GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(flat_shaded_data.size()));
    }
    if (wireframe_mode)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        glUniform1i(shvar_shading_mode, 0);
        glUniform1i(shvar_backface_highlight, 0);
        if (smooth_shading)
            glDrawElements(GL_TRIANGLES, static_cast<int>(mesh_data.indices.size()), GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(flat_shaded_data.size()));
    }
}

void GraphicsEnv::configureImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
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
    auto io = ImGui::GetIO();
    static bool is_first_run = true;

    float stats_window_height = 0;
    if (ImGui::Begin("stats & info", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::CollapsingHeader("mesh info", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {
            ImGui::LabelText("vertices", format(locale("en_US.UTF-8"), "{0:L} ({1})", summary_stats.vertices, MTVT::getMemorySize(summary_stats.vertices_bytes)).c_str());
            ImGui::LabelText("triangles", format(locale("en_US.UTF-8"), "{0:L} ({1})", summary_stats.triangles, MTVT::getMemorySize(summary_stats.indices_bytes)).c_str());
        }

        if (ImGui::CollapsingHeader("generation stats", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {
            ImGui::LabelText("resolution", "%i x %i x %i", summary_stats.cubes_x, summary_stats.cubes_y, summary_stats.cubes_z);
            ImGui::LabelText("lattice type", summary_stats.lattice_type.c_str());
            ImGui::LabelText("clustering mode", summary_stats.clustering_mode.c_str());
            ImGui::Separator();
            ImGui::LabelText("sample points", format(locale("en_US.UTF-8"), "{0:L} ({1})", summary_stats.sample_points_allocated, MTVT::getMemorySize(summary_stats.sample_points_bytes)).c_str());
            ImGui::LabelText("sample points (ideal)", format(locale("en_US.UTF-8"), "{0:L} ({1:.2f}%% stored)", summary_stats.sample_points_theoretical, summary_stats.sample_points_allocated_percent).c_str());
            ImGui::LabelText("edges", format(locale("en_US.UTF-8"), "{0:L} ({1})", summary_stats.edges_allocated, MTVT::getMemorySize(summary_stats.edges_bytes)).c_str());
            ImGui::LabelText("edges (ideal)", format(locale("en_US.UTF-8"), "{0:L} ({1:.2f}%% stored)", summary_stats.edges_theoretical, summary_stats.edges_allocated_percent).c_str());
            ImGui::LabelText("tetrahedra", format(locale("en_US.UTF-8"), "{0:L}", summary_stats.tetrahedra_computed).c_str());
            ImGui::LabelText("tetrahedra (total)", format(locale("en_US.UTF-8"), "{0:L} ({1:.2f}%% computed)", summary_stats.tetrahedra_total, summary_stats.tetrahedra_computed_percent).c_str());
        }

        if (ImGui::CollapsingHeader("timing stats", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {
            ImGui::LabelText("total", "%.8fs", summary_stats.time_total);
            ImGui::Separator();
            ImGui::LabelText("allocation", "%.8fs (%.2f%%)", summary_stats.time_allocation, summary_stats.percent_allocation);
            ImGui::LabelText("sampling", "%.8fs (%.2f%%)", summary_stats.time_sampling, summary_stats.percent_sampling);
            ImGui::LabelText("vertex", "%.8fs (%.2f%%)", summary_stats.time_vertex, summary_stats.percent_vertex);
            ImGui::LabelText("geometry", "%.8fs (%.2f%%)", summary_stats.time_geometry, summary_stats.percent_geometry);
            ImGui::LabelText("normals", "%.8fs (%.2f%%)", summary_stats.time_normals, summary_stats.percent_normals);
        }

        if (ImGui::CollapsingHeader("geometry stats", nullptr, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {
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
        }

        if (!is_first_run)
            ImGui::SetWindowPos(ImVec2(io.DisplaySize.x - (ImGui::GetWindowWidth() + 10), 10), ImGuiCond_FirstUseEver);
        stats_window_height = ImGui::GetWindowHeight();
    }
    ImGui::End();
    
    float generation_window_height = 0;
    if (ImGui::Begin("generation controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static bool clicked = true;
        clicked |= ImGui::Checkbox("update live", &update_live);
        clicked |= ImGui::SliderFloat3("min", (float*)(&param_min), -5, 5);
        clicked |= ImGui::SliderFloat3("max", (float*)(&param_max), -5, 5);
        clicked |= ImGui::SliderFloat3("offset", (float*)(&param_off), -5, 5);
        clicked |= ImGui::SliderFloat("resolution", &param_resolution, 0.002f, 2);
        const char* options[5] = { "sphere", "bump", "fbm", "cube", "bunny" };
        clicked |= ImGui::Combo("function", &param_function, options, 5);
        clicked |= ImGui::SliderFloat("threshold", &param_threshold, -2, 2);
        ImGui::LabelText("lattice type", "");
        ImGui::BeginTable("lattice type tbl", 1, ImGuiTableFlags_BordersOuter);
        ImGui::TableNextColumn();
        clicked |= ImGui::RadioButton("body centered diamond", &param_lattice, 0);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        clicked |= ImGui::RadioButton("simple cubic", &param_lattice, 1);
        ImGui::EndTable();
        ImGui::Spacing();
        ImGui::LabelText("merging mode", "");
        ImGui::BeginTable("merging mode tbl", 1, ImGuiTableFlags_BordersOuter);
        ImGui::TableNextColumn();
        clicked |= ImGui::RadioButton("none", &param_merging, 0);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        clicked |= ImGui::RadioButton("integrated", &param_merging, 1);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        clicked |= ImGui::RadioButton("post process", &param_merging, 2);
        ImGui::EndTable();
        if (update_live)
            ImGui::BeginDisabled();
        if (ImGui::Button("generate!") || (update_live && clicked))
        {
            // run the generator!
            static float(*funcs[5])(MTVT::Vector3) = { sphereFunc, bumpFunc, fbmFunc, cubeFunc, sphereFunc };
            auto result = MTVT::runBenchmark("-", 1, param_min + param_off, param_max + param_off, param_resolution, funcs[param_function], param_threshold, (MTVT::Builder::LatticeType)param_lattice, (MTVT::Builder::ClusteringMode)param_merging, 8);
            setSummary(result.first);
            setMesh(result.second, param_off);
        }
        if (update_live)
            ImGui::EndDisabled();
        clicked = false;

        if (!is_first_run)
            ImGui::SetWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        generation_window_height = ImGui::GetWindowHeight();
    }
    ImGui::End();
    if (ImGui::Begin("view controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("%f fps", io.Framerate);
        ImGui::Text("%f ms", io.DeltaTime * 1000.0f);
        ImGui::Checkbox("vsync enabled", &vsync_enabled);
        ImGui::Separator();
        ImGui::LabelText("position", "(%.2f, %.2f, %.2f)", camera_position.x, camera_position.y, camera_position.z);
        ImGui::LabelText("rotation", "(%.2f, %.2f, %.2f)", camera_euler.x, camera_euler.y, camera_euler.z);
        ImGui::Separator();
        ImGui::SliderAngle("fov", &camera_fov, 5.0f, 140.0f);
        ImGui::Checkbox("orthographic camera", &is_orthographic);
        ImGui::BeginTable("face buttons", 3);
        ImGui::TableNextColumn();
        if (ImGui::Button("FRONT", ImVec2(60, 0)))
        {
            camera_position = { 0, 2, 0 };
            camera_euler = { 90, 0, 180 };
        }
        ImGui::TableNextColumn();
        if (ImGui::Button("BACK", ImVec2(60, 0)))
        {
            camera_position = { 0, -2, 0 };
            camera_euler = { 90, 0, 0 };
        }
        ImGui::TableNextColumn();
        if (ImGui::Button("TOP", ImVec2(60, 0)))
        {
            camera_position = { 0, 0, 2 };
            camera_euler = { 0, 0, 0 };
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::Button("LEFT", ImVec2(60, 0)))
        {
            camera_position = { -2, 0, 0 };
            camera_euler = { 90, 0, -90 };
        }
        ImGui::TableNextColumn();
        if (ImGui::Button("RIGHT", ImVec2(60, 0)))
        {
            camera_position = { 2, 0, 0 };
            camera_euler = { 90, 0, 90 };
        }
        ImGui::TableNextColumn();
        if (ImGui::Button("BOTTOM", ImVec2(60, 0)))
        {
            camera_position = { 0, 0, -2 };
            camera_euler = { 180, 0, 0 };
        }
        ImGui::EndTable();
        ImGui::Separator();
        ImGui::LabelText("shading mode", "");
        ImGui::BeginTable("shading mode tbl", 1, ImGuiTableFlags_BordersOuter);
        ImGui::TableNextColumn();
        ImGui::RadioButton("unshaded", &shading_mode, 0);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::RadioButton("phong", &shading_mode, 1);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::RadioButton("position", &shading_mode, 2);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::RadioButton("normal", &shading_mode, 3);
        ImGui::EndTable();
        ImGui::Spacing();
        ImGui::ColorEdit3("shading light", (float*)(&shading_colour_a), ImGuiColorEditFlags_PickerHueWheel);
        ImGui::ColorEdit3("shading dark", (float*)(&shading_colour_b), ImGuiColorEditFlags_PickerHueWheel);
        ImGui::Spacing();
        ImGui::Checkbox("smooth shading", &smooth_shading);
        ImGui::Checkbox("wireframe overlay", &wireframe_mode);
        ImGui::Checkbox("wireframe only", &wireframe_only);
        ImGui::Spacing();
        ImGui::LabelText("backface mode", "");
        ImGui::BeginTable("backface mode tbl", 1, ImGuiTableFlags_BordersOuter);
        ImGui::TableNextColumn();
        ImGui::RadioButton("show all", &backface_mode, 0);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::RadioButton("cull backfaces", &backface_mode, 1);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::RadioButton("cull frontfaces", &backface_mode, 2);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::RadioButton("highlight backfaces", &backface_mode, 3);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::RadioButton("highlight frontfaces", &backface_mode, 4);
        ImGui::EndTable();

        if (!is_first_run)
            ImGui::SetWindowPos(ImVec2(10, generation_window_height + 20), ImGuiCond_FirstUseEver);
        generation_window_height += ImGui::GetWindowHeight();
    }
    ImGui::End();
    if (ImGui::Begin("script controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // TODO: batch and benchmarking config, including gif generator, csv export, etc
        if (ImGui::CollapsingHeader("turntable GIF creator", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {
            int size[2] = { rtt_width, rtt_height };
            gif_name.resize(128);
            ImGui::InputText("name", gif_name.data(), gif_name.size(), ImGuiInputTextFlags_CharsNoBlank);
            ImGui::InputFloat("distance", &gif_distance, 0.1f, 0.5f);
            ImGui::InputFloat("up angle", &gif_up_angle, 10.0f, 30.0f);
            ImGui::SliderInt2("resolution", size, 10, 1024);
            rtt_width = size[0];
            rtt_height = size[1];
            ImGui::SliderFloat("length (sec)", &gif_length_seconds, 0.5f, 10.0f);
            ImGui::SliderInt("frames", &gif_length_frames, 10, 300);
            if (ImGui::Button("generate"))
                renderGIF();
        }

        if (ImGui::CollapsingHeader("benchmarker", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {

        }

        if (!is_first_run)
            ImGui::SetWindowPos(ImVec2(io.DisplaySize.x - (ImGui::GetWindowWidth() + 10), stats_window_height + 20), ImGuiCond_FirstUseEver);
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    is_first_run = false;
}

void GraphicsEnv::renderGIF()
{
    if (mesh_data.indices.empty())
        return;
    initialiseRTTState();

    vector<vector<uint8_t>> frames;

    for (int i = 0; i < gif_length_frames; ++i)
    {
        float rot_angle = ((float)i / (float)gif_length_frames) * glm::two_pi<float>();
        glm::vec3 eye = glm::vec3(cos(rot_angle) - sin(rot_angle), sin(rot_angle) + cos(rot_angle), 1.0f) * gif_distance;
        eye *= glm::vec3(cos(glm::radians(gif_up_angle)), cos(glm::radians(gif_up_angle)), sin(glm::radians(gif_up_angle)));
        rtt_transform = glm::lookAt(eye, glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
        rtt_transform = glm::perspective(camera_fov, (float)rtt_width / (float)rtt_height, 0.0001f, 100.0f) * rtt_transform;
        drawRTTScene();

        frames.push_back(exportRTTResult());
    }

    GifWriter gw;
    uint32_t delay = (gif_length_seconds / gif_length_frames) * 100.0f;
    GifBegin(&gw, gif_name.c_str(), rtt_width, rtt_height, delay, true);
    for (auto& frame : frames)
        GifWriteFrame(&gw, frame.data(), rtt_width, rtt_height, delay, 8, true);
    GifEnd(&gw);

    destroyRTTState();
}

void GraphicsEnv::initialiseRTTState()
{
    glGenFramebuffers(1, &rtt_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, rtt_framebuffer);

    glGenTextures(1, &rtt_colour_texture);
    glBindTexture(GL_TEXTURE_2D, rtt_colour_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rtt_width, rtt_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glGenRenderbuffers(1, &rtt_depth_texture);
    glBindRenderbuffer(GL_RENDERBUFFER, rtt_depth_texture);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, rtt_width, rtt_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rtt_depth_texture);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rtt_colour_texture, 0);
    GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, draw_buffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        cout << "error initialising RTT framebuffer!" << endl;
        exit(-1);
    }
}

void GraphicsEnv::drawRTTScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, rtt_framebuffer);
    glViewport(0, 0, rtt_width, rtt_height);

    glUseProgram(shader_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, backface_image);
    glBindVertexArray(vertex_array_object);

    glUniformMatrix4fv(shvar_transform, 1, GL_FALSE, glm::value_ptr(rtt_transform));
    glUniform3f(shvar_shading_colour_a, shading_colour_a.r, shading_colour_a.g, shading_colour_a.b);
    glUniform3f(shvar_shading_colour_b, shading_colour_b.r, shading_colour_b.g, shading_colour_b.b);
    glUniform1i(shvar_shading_mode, 1);
    glUniform1i(shvar_backface_highlight, (backface_mode <= 2) ? 0 : (backface_mode - 2));

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (backface_mode == 0 || backface_mode > 2)
        glDisable(GL_CULL_FACE);
    else
    {
        glEnable(GL_CULL_FACE);
        glCullFace(backface_mode == 1 ? GL_BACK : GL_FRONT);
    }
    glDrawElements(GL_TRIANGLES, static_cast<int>(mesh_data.indices.size()), GL_UNSIGNED_INT, 0);
}

vector<uint8_t> GraphicsEnv::exportRTTResult()
{
    glBindTexture(GL_TEXTURE_2D, rtt_colour_texture);
    vector<uint8_t> image; image.resize(4 * rtt_width * rtt_height);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
    return image;
}

void GraphicsEnv::destroyRTTState()
{
    glDeleteRenderbuffers(1, &rtt_depth_texture);
    glDeleteTextures(1, &rtt_colour_texture);
    glDeleteFramebuffers(1, &rtt_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    int width, height; glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
}
