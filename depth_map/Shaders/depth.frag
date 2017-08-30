#version 450 core
// Interpolated values from the vertex shaders
in vec4 vpos;

layout(location = 0) out vec3 color;

void main(){
	//color = vec3(gl_FragCoord.z);
	color = vec3(vpos.z/1000.0);

	//color = vec3(1.0f);
}
