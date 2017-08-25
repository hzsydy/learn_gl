#version 450 core
layout (points) in;
layout (points, max_vertices = 1) out;

out vec4 vpos;


void draw(vec4 position)
{    
    vpos = position;
	gl_Position = vpos;
    EmitVertex();
    EndPrimitive();
}

void main() {    
    draw(gl_in[0].gl_Position);
}  