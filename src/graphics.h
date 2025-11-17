#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <chrono>
#include <vector>

#include "MTVT.h"
#include "benchmark.h"

class GraphicsEnv
{
private:
	struct Vertex
	{
		MTVT::Vector3 position;
		MTVT::Vector3 normal;
	};

public:
	// camera parameters
	glm::vec3 camera_position = { 0, -2, 0 };
	glm::vec3 camera_euler = { 90, 0, 0 };
	glm::vec3 camera_velocity;

private:
	GLFWwindow* window;

	// mesh data, buffers, and array objects
	MTVT::Mesh mesh_data;
	std::vector<Vertex> rearranged_vertex_data;
	std::vector<Vertex> flat_shaded_data;
	unsigned int vertex_buffer;
	unsigned int index_buffer;
	unsigned int vertex_array_object;
	unsigned int flat_vertex_buffer;
	unsigned int flat_vertex_array_object;

	// shader program and variables
	unsigned int shader_program;
	unsigned int shvar_transform;
	unsigned int shvar_shading_mode;
	unsigned int shvar_backface_highlight;
	unsigned int shvar_shading_colour_a;
	unsigned int shvar_shading_colour_b;

	// texture used by the shader for annotating backfaces/frontfaces
	unsigned int backface_image;

	std::chrono::steady_clock::time_point last_frame_time;

	MTVT::SummaryStats summary_stats;

	// generation parameters
	bool update_live = true;
	MTVT::Vector3 param_min = { -1, -1, -1 };
	MTVT::Vector3 param_max = { 1, 1, 1 };
	MTVT::Vector3 param_off = { 0, 0, 0 };
	float param_resolution = 0.1f;
	int param_function = 2;
	float param_threshold = 0.0f;
	int param_lattice = 0;
	int param_merging = 1;

	// view parameters
	float camera_fov = 1.57f;
	bool vsync_enabled = true;
	bool is_orthographic = false;
	int shading_mode = 1;
	bool smooth_shading = true;
	bool wireframe_mode = false;
	bool wireframe_only = false;
	int backface_mode = 0;
	glm::vec3 shading_colour_a = { 0.949f, 0.553f, 0.027f };
	glm::vec3 shading_colour_b = { 0.212f, 0.071f, 0.310f };

	// render-to-texture data
	unsigned int rtt_framebuffer;
	unsigned int rtt_colour_texture;
	unsigned int rtt_depth_texture;
	int rtt_width = 512;
	int rtt_height = 512;
	float gif_length_seconds = 8.0f;
	int gif_length_frames = 120;
	float gif_distance = 1.2f;
	float gif_up_angle = 30.0f;
	std::string gif_name = "output.gif";
	glm::mat4 rtt_transform;

public:
	bool create(int width, int height);

	void setMesh(MTVT::Mesh mesh, MTVT::Vector3 offset);
	void setSummary(MTVT::SummaryStats summary);
	bool draw();

	void destroy();

private:
	void drawMesh(glm::mat4 transform);
	void configureImGui();
	void drawImGui();

	void renderGIF();
	void initialiseRTTState();
	void drawRTTScene();
	std::vector<uint8_t> exportRTTResult();
	void destroyRTTState();
};
