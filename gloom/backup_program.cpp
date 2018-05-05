// Local headers
#include "program.hpp"
#include "gloom/gloom.hpp"
#include "gloom/shader.hpp"

#include "gloom/utilities.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "camera.hpp"

#ifdef __linux__
#include <unistd.h>
#endif

#include <algorithm>

void createCubeObject(RenderObject* object){
  object->numVertices = 4 * 6; // No shared vertices, to keep normals consistent
  object->numIndices = 3 * 2 * 6;

  object->vertices = new float[3 * object->numVertices];
  object->normals = new float[3 * object->numVertices];
  object->uvs = new float[2 * object->numVertices];

  object->indices = new uint32_t[object->numIndices];

  // Magic algorithm incoming
  for(int i = 0; i < 6; i++){
    object->indices[3 * 2 * i + 0] = 4 * i + (i % 2 ? 0 : 3);
    object->indices[3 * 2 * i + 1] = 4 * i + 1;
    object->indices[3 * 2 * i + 2] = 4 * i + 2;
    
    object->indices[3 * 2 * i + 3] = 4 * i + 1;
    object->indices[3 * 2 * i + 4] = 4 * i + (i % 2 ? 3 : 0);
    object->indices[3 * 2 * i + 5] = 4 * i + 2;

    int f_ind = i / 2;
    int s_ind = (f_ind + 1) % 3;
    int t_ind = (s_ind + 1) % 3;
    for(int j = 0; j < 4; j++){
      object->vertices[(i * 4 + j) * 3 + f_ind] = (j % 2) * 2 - 1;
      object->vertices[(i * 4 + j) * 3 + s_ind] = (j / 2) * 2 - 1;
      object->vertices[(i * 4 + j) * 3 + t_ind] = (i % 2) * 2 - 1;

      object->normals[(i * 4 + j) * 3 + f_ind] = 0;
      object->normals[(i * 4 + j) * 3 + s_ind] = 0;
      object->normals[(i * 4 + j) * 3 + t_ind] = (i % 2) * 2 - 1;

      object->uvs[(i * 4 + j) * 2 + 0] = j % 2;
      object->uvs[(i * 4 + j) * 2 + 1] = j / 2;
    }
  }
}

void destroyRenderObject(RenderObject* object){
  delete[] object->vertices;
  delete[] object->normals;
  delete[] object->uvs;

  delete[] object->indices;
}

unsigned int createVAO(int numArrays, int numElems, float** arrays, int* sizes, int numIndices, unsigned int* indices){
  unsigned int vao;
  glGenVertexArrays(1, &vao);

  glBindVertexArray(vao);

  const int maxvbos = 16;
  if(numArrays > maxvbos){
    printf("Too many arrays sent to createVAO (sent %d, max is %d)\n", numArrays, maxvbos);
    exit(-1);
  }
  unsigned int vbos[maxvbos];
  glGenBuffers(numArrays, vbos);

  for(int i = 0; i < numArrays; i++){
    glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);

    glBufferData(GL_ARRAY_BUFFER, sizes[i] * numElems * sizeof(float), arrays[i], GL_STATIC_DRAW);

    glVertexAttribPointer(i, sizes[i], GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(i);
  }

  unsigned int indexBuffer;
  glGenBuffers(1, &indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

  glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(unsigned int), indices, GL_STATIC_DRAW);

  return vao;
}

unsigned int createVAOPosAndTex(int numElems, float* vertices, float* coords, int numIndices, unsigned int* indices){
  float* arrays[] = {vertices, coords};
  int sizes[] = {3, 2};
  return createVAO(2, numElems, arrays, sizes, numIndices, indices);
}

unsigned int createVAOPosTexNormal(int numElems, float* vertices, float* coords, float* normals, int numIndices, unsigned int* indices){
  float* arrays[] = {vertices, coords, normals};
  int sizes[] = {3, 2, 3};
  return createVAO(3, numElems, arrays, sizes, numIndices, indices);
}

unsigned int createObjectVAO(const RenderObject& object){
  float* arrays[] = {object.vertices, object.uvs, object.normals};
  int sizes[] = {3, 2, 3};
  return createVAO(3, object.numVertices, arrays, sizes, object.numIndices, object.indices);
}

unsigned int createTexture(std::string filename){
  PNGImage image = loadPNGFile(filename);

  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
	       image.width, image.height, 0,
	       GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, image.pixels.data());
  glGenerateMipmap(GL_TEXTURE_2D);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
  return texture;
}

// If textureFile is non-zero, width and height are ignored
unsigned int createCubeMapTexture(int size, std::string* textureFile = 0){
  unsigned int textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); 
  
  if(textureFile){
    PNGImage image = loadPNGFile(*textureFile);
    if(image.height != image.width){
      puts((std::string("Texture ") + *textureFile + std::string(" did not have equal width and height, which are required for cubemap\n")).c_str());
    }

    for(int i = 0; i < 6; i++){
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA,
		   image.width, image.height, 0,
		   GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, image.pixels.data());
    }
  }else{
    for(int i = 0; i < 6; i++){
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA,
		   size, size, 0,
		   GL_RGBA, GL_FLOAT, NULL);
    }
  }

  
  return textureID;
}

unsigned int createFramebuffer(int width, int height){
  unsigned int framebuffer;
  glGenFramebuffersEXT(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  unsigned int depthbuffer;
  glGenRenderbuffersEXT(1, &depthbuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer);
  
  return framebuffer;
}

unsigned int createCubeFrameBuffer(int size){
  unsigned int depthCubeMap, colorCubeMap, framebuffer;
  glGenTextures(1, &depthCubeMap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  for(int i = 0; i < 6; i++){
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24,
		 size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  }

  glGenTextures(1, &colorCubeMap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, colorCubeMap);
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  for(int i = 0; i < 6; i++){
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA,
		 size, size, 0, GL_RGBA, GL_FLOAT, NULL);
  }

  glGenFramebuffersEXT(1, &framebuffer);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
  glFramebufferTextureARB(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, depthCubeMap, 0);
  glFramebufferTextureARB(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, colorCubeMap, 0);

  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

  if(glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE){
    printf("Incomplete framebuffer!\n");
    exit(-1);
  }
}

unsigned int cubeVAO;
RenderObject cubeObject;
unsigned int uniformModel;

void renderScene(float count){
  int program;
  
  glGetIntegerv(GL_CURRENT_PROGRAM, &program);
  
  glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
  uniformModel = glGetUniformLocation(program, "model");
  
  glBindVertexArray(cubeVAO);
  
  for(int i = 0 ; i < 4; i++){
    //float theta = (i  + 0.5)* M_PI / 2;
    float theta = count + i * M_PI / 2;
    glm::vec3 translation = glm::vec3(sin(theta), 0, cos(theta));
    model = glm::translate(glm::mat4(1.0f), 3.0f * translation); 
    glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
    glDrawElements(GL_TRIANGLES, cubeObject.numIndices, GL_UNSIGNED_INT, 0);  
  }
}

void runProgram(GLFWwindow* window)
{
    // Enable depth (Z) buffer (accept "closest" fragment)
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Configure miscellaneous OpenGL settings
    glEnable(GL_CULL_FACE);

    // Set default colour after clearing the colour buffer
    glClearColor(0.3f, 0.5f, 0.8f, 1.0f);

    // Enable alpha blending:
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND );

    GlCamera camera;

    unsigned int texture = createTexture("../gloom/src/gloom/diamond.png");
    glBindTextureUnit(0, texture);

    unsigned int cubeMap = createCubeMapTexture(1024);
    
    createCubeObject(&cubeObject);
    cubeVAO = createObjectVAO(cubeObject);

    Gloom::Shader shader;
    Gloom::Shader reflectionShader;
    Gloom::Shader toCubeShader;
    
    shader.makeBasicShader("../gloom/shaders/lighting.vert",
			   "../gloom/shaders/lighting.frag");

    reflectionShader.makeBasicShader("../gloom/shaders/reflection.vert",
				     "../gloom/shaders/reflection.frag");

    toCubeShader.attach("../gloom/shaders/lighting.vert");
    //toCubeShader.attach("../gloom/shaders/lighting.geom");
    toCubeShader.attach("../gloom/shaders/lighting.frag");
    toCubeShader.link();
    
    unsigned int uniformLightPosition = glGetUniformLocation(shader.get(), "lightPosition");

    unsigned int uniformView = glGetUniformLocation(shader.get(), "view");
    unsigned int uniformProjection = glGetUniformLocation(shader.get(), "projection");
    
    unsigned int cube_framebuffer = createFramebuffer(1024, 1024);

    unsigned int model_reflection_uniform = glGetUniformLocation(reflectionShader.get(), "model");
    unsigned int view_reflection_uniform = glGetUniformLocation(reflectionShader.get(), "view");
    unsigned int projection_reflection_uniform = glGetUniformLocation(reflectionShader.get(), "projection");
      
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Rendering Loop
    float count = 0;
    
    while (!glfwWindowShouldClose(window))
    {
      // Clear colour and depth buffers
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      
      float deltaTime = getTimeDeltaSeconds();
      
      // Draw your scene here
      glUseProgram(shader.get());
      
      count += deltaTime;

      float theta = count;
      glm::vec3 lightPosition = 5.0f * glm::vec3(sin(theta), 2, cos(theta));
      glUniform3f(uniformLightPosition, lightPosition.x, lightPosition.y, lightPosition.z);
      
      // Render from middle instance
      glm::mat4 projection = glm::perspective(M_PI / 4, 1.0, 0.01, 100.0);
      glm::mat4 view = glm::mat4(1.0f);
      
      
      glBindFramebuffer(GL_FRAMEBUFFER, cube_framebuffer);
      glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
      glBindTexture(GL_TEXTURE_2D, texture);
      
      for(int i = 0; i < 6; i++){
	glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(view));
	  
	  
	
	renderScene(count);
	// glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	// 		       GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
	// 		       cubeMap,
	// 		       0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			       GL_TEXTURE_2D, texture, 0);

	glBindTextureUnit(0, texture);

	if(i % 2)
	  view = glm::rotate(view, 180.f / 3.f, glm::vec3(1.f, 1.f, 1.f));

	view = view * glm::mat4(-1.0);
      }
	
      // Render from viewpoint
	
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      projection = glm::perspective(M_PI / 3, 4./3.,
				    0.01, 100.0);
      view = updateCameraTransform(window);
	
      glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));

      //renderScene(count);

      glUseProgram(reflectionShader.get());
      glBindTextureUnit(0, texture);
      glBindVertexArray(cubeVAO);

      glm::mat4 model(1.0f);
      glUniformMatrix4fv(model_reflection_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniformMatrix4fv(view_reflection_uniform, 1, GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(projection_reflection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

      glDrawElements(GL_TRIANGLES, cubeObject.numIndices, GL_UNSIGNED_INT, 0);
	
      // Handle other events
      glfwPollEvents();
      handleKeyboardInput(window);

      // Flip buffers
      glfwSwapBuffers(window);

      // In case something has happened
      printGLError();

#ifdef __linux__
      // Let the poor CPU and GPU rest
      usleep(10000);
#endif
    }
}


void handleKeyboardInput(GLFWwindow* window)
{
    // Use escape key for terminating the GLFW window
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}
