#version 400
in vec2 tex_coord;
out vec4 color;
uniform sampler2D tex_buff;
uniform vec2 offsetTex;

void main() {
    color = texture(tex_buff, tex_coord + offsetTex);
}