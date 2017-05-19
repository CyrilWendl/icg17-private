#version 330

in vec3 position;

out vec2 uv;
out float height;       // out the height to know if tree should be snowy for example


uniform mat4 MVP;

uniform float time;
uniform float tree_height;
uniform float offset_x;
uniform float offset_y;
uniform sampler2D texNoise;     // pass the terrain to compute base of the tree


void main() {

    vec3 pos = position;
    pos +=8.*vec3(-offset_x,-offset_y,1.0/8.0);     // to keep the tree in place

    uv = (position.xy + vec2(1.0)) / 2.0;
    height = float(texture(texNoise,uv).x);
    vec3 pos_3d = vec3(pos.x+sin(pos.x)*sin(time) , position.z+height, pos.y);
    gl_Position = MVP * vec4(pos_3d, 1.0);
}