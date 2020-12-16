#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
//
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
//
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
//
#include <glm/glm.hpp>
//
#include <glm/ext.hpp>

using namespace std;
using namespace gl;

static const struct {
  float x, y;
  float r, g, b;
} vertices[3] = {{-0.6f, -0.4f, 1.f, 0.f, 0.f},
                 {0.6f, -0.4f, 0.f, 1.f, 0.f},
                 {0.f, 0.6f, 0.f, 0.f, 1.f}};

static const char* vertex_shader_text =
    "#version 330\n"
    "uniform mat4 MVP;"
    // "attribute vec3 vCol;"
    // "attribute vec2 vPos;"
    "attribute vec3 vPos;"
    "attribute vec3 vNor;"
    // "varying vec3 color;"
    "varying vec3 normal;"
    "out vec3 frag_pos;"
    "void main()"
    "{"
    "  gl_Position = MVP * vec4(vPos, 1.0);"
    "  frag_pos = vPos;"
    // "  color = vCol;"
    "  normal = vNor;"
    "}";

static const char* fragment_shader_text =
    "#version 330\n"
    "uniform vec3 light_pos;"
    // "varying vec3 color;"
    "varying vec3 normal;"
    "in vec3 frag_pos;"
    "void main()"
    "{"
    // "  gl_FragColor = vec4(color, 1.0);"
    // "  gl_FragColor = vec4(normal, 1.0);"
    "  vec3 norm = normalize(normal);"
    "  vec3 light_dir = normalize(light_pos - frag_pos);"
    // "  float diffuse = max(dot(norm, light_dir), 0.0);"
    "  float diffuse = abs(dot(norm, light_dir));"
    "  vec3 diffuse_color = diffuse * vec3(1.0, 0.8, 0.1);"
    "  float diffuse_coeff = 0.5;"

    "  vec3 ambient_color = vec3(1.0, 0.8, 0.1);"
    "  float ambient_coeff = 0.3;"

    "  float specular_coeff = 0.2;"
    "  vec3 view_dir = normalize(light_pos - frag_pos);"
    "  vec3 reflect_dir = reflect(-light_dir, norm);"
    // "  float specular = pow(max(dot(view_dir, reflect_dir), 0.0), 32);"
    "  float specular = pow(abs(dot(view_dir, reflect_dir)), 6);"
    "  vec3 specular_color = specular * vec3(1.0, 1.0, 1.0);"

    "  gl_FragColor = diffuse_coeff * vec4(diffuse_color, 1.0) + ambient_coeff "
    "* vec4(ambient_color, 1.0) + specular_coeff * vec4(specular_color, 1.0);"
    "}";

glm::vec3 up{0, 0, 1};
glm::vec3 origin{0, 0, 0};
float fov = 45.0f;
float radius = 5.0f;
float altitude = 0.0f;
float azimuth = 0.0f;

int main(int argc, char* argv[]) {
  if (argc != 2) {
    cout << "usage:\n" << argv[0] << " <stl file>\n";
    return -1;
  }

  fstream file{argv[1], ios::in | ios::binary};
  // Ignore header.
  file.ignore(80);
  uint32_t stl_size;
  file.read(reinterpret_cast<char*>(&stl_size), sizeof(uint32_t));

  vector<glm::vec3> triangles(6 * stl_size);
  for (size_t i = 0; i < stl_size; ++i) {
    // file.ignore(12);
    glm::vec3 normal;
    file.read(reinterpret_cast<char*>(&normal), sizeof(glm::vec3));
    for (size_t j = 0; j < 3; ++j) {
      glm::vec3 vertex;
      file.read(reinterpret_cast<char*>(&vertex), sizeof(glm::vec3));
      triangles[2 * (3 * i + j) + 0] = vertex;
      triangles[2 * (3 * i + j) + 1] = normal;
    }
    file.ignore(2);
  }

  glm::vec3 aabb_min{triangles[0]};
  glm::vec3 aabb_max{triangles[0]};
  for (size_t i = 2; i < triangles.size(); i += 2) {
    aabb_min = min(aabb_min, triangles[i]);
    aabb_max = max(aabb_max, triangles[i]);
  }
  origin = 0.5f * (aabb_max + aabb_min);
  radius = 0.5f * length(aabb_max - aabb_min) *
           (1.0f / tan(0.5f * fov * M_PI / 180.0f));

  // cout << "stl size = " << stl_size << '\n';
  // for (size_t i = 0; i < 10; ++i)
  //   cout << setw(10) << triangles[i].x << setw(10) << triangles[i].y <<
  //   setw(10)
  //        << triangles[i].z << '\n';

  glfwSetErrorCallback([](int error, const char* description) {
    throw runtime_error{"GLFW Error " + to_string(error) + ": " + description};
  });
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);

  auto window = glfwCreateWindow(640, 480, "Simple example", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glbinding::initialize(glfwGetProcAddress);

  glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode,
                                int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
      glfwSetWindowShouldClose(window, GLFW_TRUE);
  });
  glfwSetScrollCallback(window, [](GLFWwindow* window, double x, double y) {
    radius *= exp(-0.1f * float(y));
  });

  GLuint vertex_array;
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);

  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBufferData(GL_ARRAY_BUFFER, triangles.size() * sizeof(glm::vec3),
               triangles.data(), GL_STATIC_DRAW);

  auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
  glCompileShader(vertex_shader);

  auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
  glCompileShader(fragment_shader);

  auto program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  auto mvp_location = glGetUniformLocation(program, "MVP");
  auto light_pos_location = glGetUniformLocation(program, "light_pos");
  auto vpos_location = glGetAttribLocation(program, "vPos");
  auto vnor_location = glGetAttribLocation(program, "vNor");

  // auto vcol_location = glGetAttribLocation(program, "vCol");

  // glEnableVertexAttribArray(vpos_location);
  // glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE,
  //                       sizeof(vertices[0]), (void*)0);
  // glEnableVertexAttribArray(vcol_location);
  // glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE,
  //                       sizeof(vertices[0]), (void*)(sizeof(float) * 2));
  glEnableVertexAttribArray(vpos_location);
  glVertexAttribPointer(vpos_location, 3, GL_FLOAT, GL_FALSE,
                        2 * sizeof(glm::vec3), (void*)0);
  glEnableVertexAttribArray(vnor_location);
  glVertexAttribPointer(vnor_location, 3, GL_FLOAT, GL_FALSE,
                        2 * sizeof(glm::vec3), (void*)sizeof(glm::vec3));

  glm::vec2 old_mouse_pos{};
  glm::vec2 mouse_pos{};

  glEnable(GL_DEPTH_TEST);

  while (!glfwWindowShouldClose(window)) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    auto ratio = width / (float)height;

    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::vec3 camera{cos(altitude) * cos(azimuth), cos(altitude) * sin(azimuth),
                     sin(altitude)};
    camera *= radius;
    const auto v = glm::lookAt(camera + origin, origin, up);
    const auto camera_right = normalize(cross(-camera, up));
    const auto camera_up = normalize(cross(camera_right, -camera));
    const float pixel_size = 2.0f * tan(0.5f * fov * M_PI / 180.0f) / height;

    old_mouse_pos = mouse_pos;
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    mouse_pos = glm::vec2{xpos, ypos};
    const auto mouse_move = mouse_pos - old_mouse_pos;

    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (state == GLFW_PRESS) {
      altitude += mouse_move.y * 0.01;
      azimuth -= mouse_move.x * 0.01;
      constexpr float bound = M_PI_2 - 1e-5f;
      // if (altitude >= bound) altitude = bound;
      // if (altitude <= -bound) altitude = -bound;
      altitude = clamp(altitude, -bound, bound);
    }
    state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    if (state == GLFW_PRESS) {
      const auto scale = 1.3f * pixel_size * length(camera);
      origin += -scale * mouse_move.x * camera_right +
                scale * mouse_move.y * camera_up;
    }

    glm::mat4x4 m{1.0f};
    // m = rotate(m, (float)glfwGetTime(),
    //            glm::vec3(1 / sqrt(3), 1 / sqrt(3), 1 / sqrt(3)));
    // glm::mat4 v = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5));
    glm::mat4 p = glm::perspective(fov, ratio, 0.1f, 10000.f);
    glm::mat4 mvp = p * v * m;

    glUseProgram(program);
    glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3fv(light_pos_location, 1, glm::value_ptr(camera + origin));
    // glDrawArrays(GL_TRIANGLES, 0, 3);
    glDrawArrays(GL_TRIANGLES, 0, triangles.size() / 2);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
}