#version 330

in vec2 uv;

layout (location = 0) out vec3 color;
layout (location = 1) out vec3 color2;

uniform sampler2D tex;
uniform sampler2D tex_1;

uniform float tex_width;
uniform float tex_height;

uniform int pass;
uniform int size;

uniform float kernel[128];

void main() {
     vec3 gauss_color = vec3(0.0);
     float tot_weight = 0.0f;
     //if (pass == 0) {
         for(int i=-size; i <= size; i++) {
             gauss_color += texture(tex, uv+vec2(i/tex_width,0)).rgb * kernel[i];
             tot_weight += kernel[i];
         }

         color = gauss_color / tot_weight;
     /* } else {
          for (int i = -size; i <= size; i++) {
             gauss_color += texture(tex_1, uv+vec2(0,i/tex_height)).rgb * kernel[i];
             tot_weight += kernel[i];
           }
          color2 = gauss_color / tot_weight;
      }*/
}

