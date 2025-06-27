#version 400
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texC;

out vec2 tex_coord;
uniform mat4 model;
uniform mat4 projection;

void main() {
    tex_coord = vec2(texC.s, 1.0f-texC.t);
    gl_Position = projection * model * vec4(position, 1.0f);
}
