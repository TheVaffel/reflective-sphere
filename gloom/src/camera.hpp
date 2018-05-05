#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define _USE_MATH_DEFINES
#include <cmath>

// Local headers
#include "program.hpp"
#include "gloom/gloom.hpp"

class GlCamera{
  glm::mat4 transform;
  glm::mat4 projection;

public:
  GlCamera();

  void setTransform(glm::mat4 transform);
  glm::mat4 get();
};

glm::mat4 updateCameraTransform(GLFWwindow* window);
