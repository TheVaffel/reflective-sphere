#include "camera.hpp"


GlCamera::GlCamera(){
  projection = glm::perspective(M_PI / 3, 4.f/3.,
				 0.01, 100.0);
}


void GlCamera::setTransform(glm::mat4 newTrans){
  transform = newTrans;
}

glm::mat4 GlCamera::get(){
  return projection * transform;
}

int getUpTurn(GLFWwindow* window){
  
  if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS){
      return 1;
  }else if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS){
    return -1;
  }
  
  return 0;
}

int getRightTurn(GLFWwindow* window){
  if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS){
      return 1;
  }else if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS){
    return -1;
  }
  
  return 0;
}

int getMovementX(GLFWwindow* window){
  if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
      return 1;
  }else if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
    return -1;
  }
  
  return 0;
}

int getMovementZ(GLFWwindow* window){
  if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
      return 1;
  }else if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
    return -1;
  }
  
  return 0;
}


float yaw = -M_PI/4, pitch = -M_PI/5;
glm::vec4 translation(-5, 5, 5, 0);

glm::mat4 updateCameraTransform(GLFWwindow* window){
  const float turnSpeed = 0.01;
  const float moveSpeed = 0.1;
	
  pitch = std::max(-1.5f, std::min(1.5f, pitch + turnSpeed*getUpTurn(window)));
  yaw = yaw - turnSpeed * getRightTurn(window);

  glm::mat4 rotation = glm::rotate(glm::mat4(), yaw, glm::vec3(0, 1, 0))
    * glm::rotate(glm::mat4(), pitch, glm::vec3(1, 0, 0));
	
  translation += moveSpeed * getMovementX(window) * (rotation * glm::vec4(1, 0, 0, 0));
  translation -= moveSpeed * getMovementZ(window) * (rotation * glm::vec4(0, 0, 1, 0));

  glm::mat4 view = glm::mat4();

  view = glm::translate(view, glm::vec3(translation * rotation));
  view = rotation * view;
	
  view = glm::inverse(view);

  return view;
}
