#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <chrono>

#include "MTVT.h"
#include "benchmark.h"

class GraphicsEnv
{
public:
	glm::vec3 camera_position = { 0, -2, 0 };
	glm::vec3 camera_euler = { 90, 0, 0 };
	glm::vec3 camera_velocity;

	float camera_fov = 1.57f;
	bool vsync_enabled = true;
	bool is_orthographic = false;
	int shading = 1;
	bool smooth_shading = true;
	bool wireframe_mode = false;
	bool wireframe_only = false;
	int backface_mode = 0;

private:
	GLFWwindow* window;

	MTVT::Mesh mesh_data;
	unsigned int vertex_buffer;
	unsigned int index_buffer;

	unsigned int shader_program;
	unsigned int shvar_transform;
	unsigned int shvar_shading_mode;
	unsigned int shvar_backface_highlight;
	unsigned int shvar_smooth_shading;
	unsigned int vertex_array_object;

	std::chrono::steady_clock::time_point last_frame_time;

	MTVT::SummaryStats summary_stats;

public:
	bool create(int width, int height);

	void setMesh(MTVT::Mesh mesh);
	void setSummary(MTVT::SummaryStats summary);
	bool draw();

	void destroy();

private:
	void configureImGui();
	void drawImGui();
};
