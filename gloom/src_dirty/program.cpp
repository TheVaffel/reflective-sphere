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

#define PE() {printf("OpenGL Error at %s, line %d?\n", __FILE__, __LINE__); printGLError();printf("End\n");}

unsigned int createObjectVAO(const RenderObject& object);
void computeTangentAndBitangent(RenderObject& object);

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

  computeTangentAndBitangent(*object);
  
  object->vao = createObjectVAO(*object);
}

void createSphereObject(RenderObject* object, float size, int resolution){
  // We will have repeated vertices to make the texture nice
  object->numVertices = (resolution + 1) * (resolution - 1) + 2; // Cylindrical grid plus top and bottom vertex
  object->numIndices = 3 * (resolution * 2 + (resolution - 2) * resolution * 2);

  
  object->vertices = new float[3 * object->numVertices];
  object->normals = new float[3 * object->numVertices];
  object->uvs = new float[2 * object->numVertices];

  object->indices = new uint32_t[object->numIndices];

  for(unsigned int i = 0; i < object->numVertices; i++){
    object->vertices[3 * i + 0] = 0;
    object->vertices[3 * i + 1] = 0;
    object->vertices[3 * i + 2] = 0;

    object->normals[3 * i + 0] = 0;
    object->normals[3 * i + 1] = 0;
    object->normals[3 * i + 2] = 0;

    object->uvs[2 * i + 0] = 0;
    object->uvs[2 * i + 1] = 0;
  }
  
  for(int i = 0; i < resolution - 2; i++){
    for(int j = 0; j < resolution ; j++){
      // Create grid, column j, row i
      object->indices[3 * 2 * resolution * i + 3 * 2 * j + 0] = (i + 0) * (resolution + 1) + j + 0;
      object->indices[3 * 2 * resolution * i + 3 * 2 * j + 1] = (i + 0) * (resolution + 1) + j + 1;
      object->indices[3 * 2 * resolution * i + 3 * 2 * j + 2] = (i + 1) * (resolution + 1) + j + 0;

      object->indices[3 * 2 * resolution * i + 3 * 2 * j + 3] = (i + 0) * (resolution + 1) + j + 1;
      object->indices[3 * 2 * resolution * i + 3 * 2 * j + 4] = (i + 1) * (resolution + 1) + j + 1;
      object->indices[3 * 2 * resolution * i + 3 * 2 * j + 5] = (i + 1) * (resolution + 1) + j + 0;
    }
  }

  for(int i = 0; i < resolution; i++){
    // Connect to bottom point
    object->indices[3 * 2 * resolution * (resolution - 2) + 3 * i + 0] = i + 1;
    object->indices[3 * 2 * resolution * (resolution - 2) + 3 * i + 1] = i;
    object->indices[3 * 2 * resolution * (resolution - 2) + 3 * i + 2] = (resolution + 1) * (resolution - 1);
    
    // Connect to top point
    object->indices[3 * 2 * resolution * (resolution - 2) + 3 * (resolution + i) + 0] =
      (resolution + 1) * (resolution - 2) + i;
    object->indices[3 * 2 * resolution * (resolution - 2) + 3 * (resolution + i) + 1] =
      (resolution + 1) * (resolution - 2) + i + 1;
    object->indices[3 * 2 * resolution * (resolution - 2) + 3 * (resolution + i) + 2] =
      (resolution + 1) * (resolution - 1) + 1;
  }

  // And now to generate the vertices:

  for(int i = 0; i < resolution - 1; i++){
    float verticalAngle = (M_PI * (i + 1)) / resolution - M_PI / 2;
    float ch = cos(verticalAngle);
    for(int j = 0; j < resolution + 1; j++){
      float horizontalAngle = 2 * M_PI * j / resolution;
      object->normals[3 * (i * (resolution + 1) + j) + 0] = ch * sin(horizontalAngle);
      object->normals[3 * (i * (resolution + 1) + j) + 1] = sin(verticalAngle);
      object->normals[3 * (i * (resolution + 1) + j) + 2] = ch * cos(horizontalAngle);

      // Calculate vertices as size * normals
      object->vertices[3 * (i * (resolution + 1) + j) + 0] =
	size * object->normals[3 * (i * (resolution + 1) + j) + 0];
      object->vertices[3 * (i * (resolution + 1) + j) + 1] =
	size * object->normals[3 * (i * (resolution + 1) + j) + 1];
      object->vertices[3 * (i * (resolution + 1) + j) + 2] =
	size * object->normals[3 * (i * (resolution + 1) + j) + 2];

      object->uvs[2 * (i * (resolution + 1) + j) + 0] = ((float)j) / resolution;
      object->uvs[2 * (i * (resolution + 1) + j) + 1] = ((float)(i + 1)) / resolution;
    }
  }

  // Top and bottom 
  
  object->vertices[3 * (resolution + 1) * (resolution - 1) + 0] = 0;
  object->vertices[3 * (resolution + 1) * (resolution - 1) + 1] = -size;
  object->vertices[3 * (resolution + 1) * (resolution - 1) + 2] = 0;

  object->vertices[3 * (resolution + 1) * (resolution - 1) + 3] = 0;
  object->vertices[3 * (resolution + 1) * (resolution - 1) + 4] = size;
  object->vertices[3 * (resolution + 1) * (resolution - 1) + 5] = 0;

  object->normals[3 * (resolution + 1) * (resolution - 1) + 0] = 0;
  object->normals[3 * (resolution + 1) * (resolution - 1) + 1] = -1;
  object->normals[3 * (resolution + 1) * (resolution - 1) + 2] = 0;
  
  object->normals[3 * (resolution + 1) * (resolution - 1) + 3] = 0;
  object->normals[3 * (resolution + 1) * (resolution - 1) + 4] = 1;
  object->normals[3 * (resolution + 1) * (resolution - 1) + 5] = 0;

  object->uvs[2 * (resolution + 1) * (resolution - 1) + 0] = 0.5f;
  object->uvs[2 * (resolution + 1) * (resolution - 1) + 1] = 0.0f;

  object->uvs[2 * (resolution + 1) * (resolution - 1) + 2] = 0.5f;
  object->uvs[2 * (resolution + 1) * (resolution - 1) + 3] = 1.0f;

  // Compute tangents and bitangents (duh)

  computeTangentAndBitangent(*object);
  
  // Create VAO
  
  object->vao = createObjectVAO(*object);
}

void destroyRenderObject(RenderObject* object){
  delete[] object->vertices;
  delete[] object->normals;
  delete[] object->uvs;

  delete[] object->tangents;
  delete[] object->bitangents;
  
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

void computeTangentAndBitangent(RenderObject& object){
  float* t = new float[object.numVertices * 3];
  float* b = new float[object.numVertices * 3];

  for(unsigned int i = 0; i < object.numVertices * 3; i++){
    t[i] = 0;
    b[i] = 0;
  }

  for(unsigned int i = 0; i < object.numIndices/3; i++){
    int vert1 = object.indices[3 * i];
    int vert2 = object.indices[3 * i + 1];
    int vert3 = object.indices[3 * i + 2];

    float* vertices = object.vertices;
    float* textureCoordinates = (float*)object.uvs;

    glm::vec3 pos1(vertices[3 * vert1],
		   vertices[3 * vert1 + 1],
		   vertices[3 * vert1 + 2]);
    glm::vec3 pos2(vertices[3 * vert2],
		   vertices[3 * vert2 + 1],
		   vertices[3 * vert2 + 2]);
    glm::vec3 pos3(vertices[3 * vert3],
		   vertices[3 * vert3 + 1],
		   vertices[3 * vert3 + 2]);

    glm::vec2 uv1(textureCoordinates[2 * vert1],
		  textureCoordinates[2 * vert1 + 1]);
    glm::vec2 uv2(textureCoordinates[2 * vert2],
		  textureCoordinates[2 * vert2 + 1]);
    glm::vec2 uv3(textureCoordinates[2 * vert3],
		  textureCoordinates[2 * vert3 + 1]);
    
    glm::vec3 edge1 = pos2 - pos1;
    glm::vec3 edge2 = pos3 - pos1;
    glm::vec2 deltaUV1 = uv2 - uv1;
    glm::vec2 deltaUV2 = uv3 - uv1; 

    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    glm::vec3 tangent;
    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
    tangent = glm::normalize(tangent);
    
    t[3 * vert1 + 0] += tangent.x; t[3 * vert2 + 0] += tangent.x; t[3 * vert3 + 0] += tangent.x;
    t[3 * vert1 + 1] += tangent.y; t[3 * vert2 + 1] += tangent.y; t[3 * vert3 + 1] += tangent.y;
    t[3 * vert1 + 2] += tangent.z; t[3 * vert2 + 2] += tangent.z; t[3 * vert3 + 2] += tangent.z;


    glm::vec3 bitangent;
    bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
    bitangent = glm::normalize(bitangent);

    b[3 * vert1 + 0] += bitangent.x; b[3 * vert2 + 0] += bitangent.x; b[3 * vert3 + 0] += bitangent.x;
    b[3 * vert1 + 1] += bitangent.y; b[3 * vert2 + 1] += bitangent.y; b[3 * vert3 + 1] += bitangent.y;
    b[3 * vert1 + 2] += bitangent.z; b[3 * vert2 + 2] += bitangent.z; b[3 * vert3 + 2] += bitangent.z;
  }

  // normalize
  
  for(unsigned int i = 0; i < object.numVertices; i++){
    float sqsumt = 0.0f, sqsumb = 0.0f;
    for(int j = 0; j < 3; j++){
      sqsumt += t[3 * i + j] * t[3 * i + j];
      sqsumb += b[3 * i + j] * b[3 * i + j];
    }
    if(sqsumt < 0.00001f){
      sqsumt = 1.0f;
    }
    if(sqsumb < 0.00001f){
      sqsumb = 1.0f;
    }
    float ilt = 1.f/sqrt(sqsumt);
    float ilb = 1.f/sqrt(sqsumb);

    for(int j = 0; j < 3; j++){
      t[3 * i + j] *= ilt;
      b[3 * i + j] *= ilb;
    }
    
    
  }

  object.tangents = t;
  object.bitangents = b;
  
  // *ot = t;
  // *ob = b;
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
  float* arrays[] = {object.vertices, object.uvs, object.normals, object.tangents, object.bitangents};
  int sizes[] = {3, 2, 3, 3, 3};
  return createVAO(5, object.numVertices, arrays, sizes, object.numIndices, object.indices);
}

unsigned int createTexture(std::string filename, unsigned int* width = 0, unsigned int* height = 0){
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
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  if(width != 0){
    *width = image.width;
  }
  
  if(height != 0){
    *height = image.height;
  }
  
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

void createCubeFrameBuffer(int size, unsigned int* framebuffer, unsigned int* colorTexture){
  glGenTextures(1, colorTexture);
  glBindTexture(GL_TEXTURE_CUBE_MAP, *colorTexture);
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  for(int i = 0; i < 6; i++){
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA,
		 size, size, 0, GL_RGBA, GL_FLOAT, NULL);
  }

  *framebuffer = createFramebuffer(size, size);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			 GL_TEXTURE_CUBE_MAP_POSITIVE_X, *colorTexture, 0);


  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
    printf("Incomplete framebuffer!\n");
    exit(-1);
  }
}

std::string vstr(glm::vec3 vec){
  char buff[256];
  sprintf(buff, "[%f, %f, %f]", vec[0], vec[1], vec[2]);
  return std::string(buff);
}

RenderObject cubeObject;
RenderObject sphereObject;
unsigned int uniformModel;
const float ball_radius = 1.0f;

unsigned int normalTexture;
Gloom::Shader* normalTextureChangeShader;
unsigned int normal_texture_framebuffer;
unsigned int normal_texture_size;

glm::mat4 view;

void changeNormals(glm::vec3 collision){
  glBindTexture(GL_TEXTURE_2D, normalTexture);
  glUseProgram(normalTextureChangeShader->get());
  unsigned int collisionUniform = glGetUniformLocation(normalTextureChangeShader->get(), "collisionPoint");
  unsigned int textureSizeUniform = glGetUniformLocation(normalTextureChangeShader->get(), "texture_size");
  // unsigned int viewUniform = glGetUniformLocation(normalTextureChangeShader->get(), "view");

  glUniform3f(collisionUniform, collision.x, collision.y, collision.z);
  glUniform1f(textureSizeUniform, (float)normal_texture_size);

  // glUniformMatrix4fv(viewUniform, 1, GL_FALSE, glm::value_ptr(view));

  glBindFramebuffer(GL_FRAMEBUFFER, normal_texture_framebuffer);
  glViewport(0, 0, normal_texture_size, normal_texture_size); // Oh boy
  glBindVertexArray(sphereObject.vao);
  glDisable(GL_DEPTH_TEST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			 GL_TEXTURE_2D, normalTexture, 0);
  glBindTextureUnit(0, normalTexture);
  
  glDrawElements(GL_TRIANGLES,
		 sphereObject.numIndices,
		 GL_UNSIGNED_INT, 0);
  glEnable(GL_DEPTH_TEST);
  printf("Hit ball!\n");
}

void shoot(const glm::vec3& position, const glm::vec3& direction){
  glm::vec3 dr = -position; // Ball center - position

  printf("Position: %s, direction %s\n", vstr(position).c_str(), vstr(direction).c_str());

  glm::vec3 q = glm::cross(dr, direction);
  float absq = glm::length(q);
  if(absq > ball_radius){;
    return;
  }
  
  float k = sqrt(ball_radius * ball_radius - absq * absq);
  float l = glm::dot(direction, dr);
  if(l < 0){
    //printf("L was %f, aborting\n", l);
    return;
  }

  glm::vec3 collision_point = position + direction * (l - k);

  printf("Collision point was %s\n", vstr(collision_point).c_str());
  changeNormals(collision_point);
}


void drawObject(const RenderObject& object){
  glDrawElements(GL_TRIANGLES, object.numIndices, GL_UNSIGNED_INT, 0);
}

void renderScene(float count){
  int program;

  glGetIntegerv(GL_CURRENT_PROGRAM, &program);

  glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
  uniformModel = glGetUniformLocation(program, "model");
  
  glBindVertexArray(sphereObject.vao);

  for(int i = 0 ; i < 4; i++){
    // float theta = (i  + 0.5)* M_PI / 2;
    float theta = count + i * M_PI / 2;
    glm::vec3 translation = glm::vec3(sin(theta), cos(2.1329 * theta) * 0.2f, cos(theta));
    model = glm::translate(glm::mat4(1.0f), 4.0f * translation); 
    glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
    glDrawElements(GL_TRIANGLES,
		   sphereObject.numIndices,
		   GL_UNSIGNED_INT, 0);  
  }

  model = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0, -8, 0)), glm::vec3(5, 5, 5));
  glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
  glBindVertexArray(cubeObject.vao);
  glDrawElements(GL_TRIANGLES,
		 cubeObject.numIndices,
		 GL_UNSIGNED_INT, 0);
}

void runProgram(GLFWwindow* window)
{
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  
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
  normalTexture = createTexture("../gloom/src/pics/flat_normals.png", &normal_texture_size);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  // glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
  glBindTextureUnit(0, texture);
    
  //createCubeObject(&sphereObject);
  createSphereObject(&sphereObject, ball_radius, 50);

  createCubeObject(&cubeObject);
  //RenderObject sphereObject;
  //createSphereObject(&sphereObject, 1.0f , 10);
   
  Gloom::Shader shader;
  Gloom::Shader reflectionShader;

  // For shooting projectiles
  const float originalCooldown = 1.0f;
  float cooldown = 0.0f;
  
    
  shader.makeBasicShader("../gloom/shaders/lighting.vert",
			 "../gloom/shaders/lighting.frag");

  reflectionShader.makeBasicShader("../gloom/shaders/reflection.vert",
				   "../gloom/shaders/reflection.frag");

  Gloom::Shader normalTextureChangeShaderObj;
  normalTextureChangeShader = &normalTextureChangeShaderObj;
  normalTextureChangeShader->makeBasicShader("../gloom/shaders/normal_changing.vert",
				      "../gloom/shaders/normal_changing.frag");

    
  unsigned int uniformLightPosition = glGetUniformLocation(shader.get(), "lightPosition");

  unsigned int uniformView = glGetUniformLocation(shader.get(), "view");
  unsigned int uniformProjection = glGetUniformLocation(shader.get(), "projection");
  
  unsigned int cube_framebuffer, cube_texture;
  createCubeFrameBuffer(256, &cube_framebuffer, &cube_texture);

  normal_texture_framebuffer = createFramebuffer(normal_texture_size, normal_texture_size);
  glBindFramebuffer(GL_FRAMEBUFFER, normal_texture_framebuffer);
  glBindTexture(GL_TEXTURE_2D, normalTexture);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			 GL_TEXTURE_2D, normalTexture, 0);

    
  unsigned int model_reflection_uniform = glGetUniformLocation(reflectionShader.get(), "model");
  unsigned int view_reflection_uniform = glGetUniformLocation(reflectionShader.get(), "view");
  unsigned int projection_reflection_uniform = glGetUniformLocation(reflectionShader.get(), "projection");

  unsigned int light_position_reflection_uniform = glGetUniformLocation(reflectionShader.get(), "lightPosition");
    
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glm::mat4 rotArray[6];
  for(int i = 0; i < 6; i++){
    rotArray[i] = glm::mat4(1.0f);
  }

  // View matrices for six different directions
    
  rotArray[0][2][0] = -1.0f; rotArray[0][0][0] = 0.0f;
  rotArray[0][0][2] = 1.0f; rotArray[0][2][2] = 0.0f;

  rotArray[1][2][0] = 1.0f; rotArray[1][0][0] = 0.0f;
  rotArray[1][0][2] = -1.0f; rotArray[1][2][2] = 0.0f;

  rotArray[2][2][1] = 1.0f; rotArray[2][1][1] = 0.0f;
  rotArray[2][1][2] = -1.0f; rotArray[2][2][2] = 0.0f;

  rotArray[3][2][1] = -1.0f; rotArray[3][1][1] = 0.0f;
  rotArray[3][1][2] = 1.0f; rotArray[3][2][2] = 0.0f;

  rotArray[4][2][2] = 1.0f;
  rotArray[4][0][0] = 1.0f;

  rotArray[5][2][2] = -1.0f;
  rotArray[5][0][0] = -1.0f;
    
  //glUseProgram(toCubeShader.get());
  //glUniformMatrix4fv(views_cube_uniform, 6, GL_FALSE, glm::value_ptr(rotArray[0]));
    
  // Rendering Loop
  float count = 0;
  int framenum = 0;
    
  while (!glfwWindowShouldClose(window))
    {
      framenum++;
      float deltaTime = getTimeDeltaSeconds();
      
      // Draw your scene here
      glUseProgram(shader.get());
      
      count += deltaTime;

      float theta = count;
      glm::vec3 lightPosition = 5.0f * glm::vec3(sin(theta), 1.5f, cos(theta));
      glUniform3f(uniformLightPosition, lightPosition.x, lightPosition.y, lightPosition.z);
      
      // Render from middle instance
      glm::mat4 projection = glm::perspective(M_PI / 2, 1.0, 0.01, 100.0);
      projection = glm::translate(glm::scale(projection, 1.f * glm::vec3(-1.f, -1.f, 1.f)),
				  glm::vec3(-.0f, -.0f, 0.0f));

      glUseProgram(shader.get());
      //glUseProgram(toCubeShader.get());
      glBindFramebuffer(GL_FRAMEBUFFER, cube_framebuffer);
      glViewport(0, 0, 256, 256);
      
      //renderScene(count);

      
      
      
      for(int i = 0; i < 6; i++){
	view = rotArray[i];
	
	glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));

	
	glBindTextureUnit(0, texture);
	
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	 		       GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
	 		       cube_texture,
	 		       0);

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindTextureUnit(0, texture);
	renderScene(count);

      }
	
      // Render from viewpoint
      
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, width, height);
      
      // Clear colour and depth buffers
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      
      glUseProgram(shader.get());
      projection = glm::perspective(M_PI / 3, 4./3.,
				    0.01, 100.0);
      view = updateCameraTransform(window);
	
      glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));

      renderScene(count);

      glUseProgram(reflectionShader.get());
      glBindTexture(GL_TEXTURE_CUBE_MAP, cube_texture);
      glBindTextureUnit(0, cube_texture);

      glBindTexture(GL_TEXTURE_2D, normalTexture);
      glBindTextureUnit(1, normalTexture);
      glBindVertexArray(sphereObject.vao);

      glm::mat4 model(1.0f);
      glUniformMatrix4fv(model_reflection_uniform, 1, GL_FALSE, glm::value_ptr(model));
      glUniformMatrix4fv(view_reflection_uniform, 1, GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(projection_reflection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

      glUniform3f(light_position_reflection_uniform, lightPosition.x, lightPosition.y, lightPosition.z);
      
      glDrawElements(GL_TRIANGLES, sphereObject.numIndices, GL_UNSIGNED_INT, 0);

      // Projectile shooting
      cooldown = std::max(0.0f, cooldown - deltaTime);
      if(
	 //framenum == 12 ||
	 (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && cooldown == 0)){
	shoot( - glm::transpose(glm::mat3(view)) *  glm::vec3(view[3]),
	       glm::transpose(glm::mat3(view)) * glm::vec3(0.0, 0.0, -1.0));
	cooldown = originalCooldown;
      }else{
	//printf("Not shooting\n");
      }
	
      // Handle other events
      glfwPollEvents();
      handleKeyboardInput(window);

      // Flip buffers
      glfwSwapBuffers(window);

      // In case something has happened
      printGLError();
      //while(glfwGetKey(window, GLFW_KEY_SPACE) != GLFW_PRESS){
#ifdef __linux__
      // Let the poor CPU and GPU rest
      usleep(10000);
      glfwPollEvents();
#endif
      //}

      //usleep(50000);
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
