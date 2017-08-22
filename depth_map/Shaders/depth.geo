#version 450 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

out vec4 vpos;

uniform float patchsize;

void draw(vec4 position)
{    
    vpos = position + vec4(-patchsize, -patchsize, 0.0, 0.0);    // 1:bottom-left
	gl_Position = vpos;
    EmitVertex();   
    vpos = position + vec4( patchsize, -patchsize, 0.0, 0.0);    // 2:bottom-right
	gl_Position = vpos;
    EmitVertex();
    vpos = position + vec4(-patchsize,  patchsize, 0.0, 0.0);    // 3:top-left
	gl_Position = vpos;
    EmitVertex();
    vpos = position + vec4( patchsize,  patchsize, 0.0, 0.0);    // 4:top-right
	gl_Position = vpos;
    EmitVertex();
    EndPrimitive();
}

void main() {    
    draw(gl_in[0].gl_Position);
}  