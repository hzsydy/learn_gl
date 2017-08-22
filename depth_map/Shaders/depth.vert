#version 450 core
// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vpos_modelspace;
// Values that stay constant for the whole mesh.
uniform mat4 MVP;

void main(){
  vec4 vpos = MVP * vec4(vpos_modelspace,1);
  gl_Position = vpos;
}
