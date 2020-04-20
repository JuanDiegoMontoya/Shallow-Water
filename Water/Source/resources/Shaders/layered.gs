#version 450

layout(points) in;
layout(points, max_vertices = 1) out;
uniform mat4 projectionMatrix;

//out vec3 fsotexCoord;

out vec4 colorPoint;

void main(void)
{
    int i, layer;
    for (layer = 0; layer < 6; layer++)
    {
        gl_Layer = layer;
        for (i = 0; i < gl_in.length(); i++)
        {
            gl_Position = gl_in[0].gl_Position;//vec4(, 1);//projectionMatrix *
            colorPoint = vec4(1, 0, 0, 1);
            EmitVertex();
        }
        EndPrimitive();
    }
}