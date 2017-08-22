#version 450 core
// Interpolated values from the vertex shaders
in vec4 vpos;
uniform float near;
uniform float far;

out vec3 color;

void main(){
	//color = vec3(gl_FragCoord.z);
	//color = vec3(vpos.z);

	color = vec3(1.0f);
}
