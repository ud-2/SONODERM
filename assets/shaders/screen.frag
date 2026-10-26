#version 330 core

out vec4 frag_color;

uniform sampler2D u_screen_texture;
uniform vec2 u_resolution;

void main() {
    // Normalized coordinates from 0.0 to 1.0
    vec2 uv = gl_FragCoord.xy / u_resolution;

    frag_color = texture(u_screen_texture, uv);
}