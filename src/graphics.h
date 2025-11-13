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

private:
	GLFWwindow* window;

	MTVT::Mesh mesh_data;
	unsigned int vertex_buffer;
	unsigned int index_buffer;

	unsigned int shader_program;
	unsigned int shu_transform_location;
	unsigned int shu_is_inverted_location;
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
