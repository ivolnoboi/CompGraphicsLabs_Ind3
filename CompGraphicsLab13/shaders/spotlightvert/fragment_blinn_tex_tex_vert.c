#version 330 core

in struct Vertex {
	vec2 texcoord;
	vec3 normal;
	vec3 lightDir;
	vec3 viewDir;
	float distance;
} Vert;
uniform sampler2D ourTexture1;
uniform sampler2D ourTexture2;
in vec4 var_color;
void main() 
{
	gl_FragColor = var_color * mix(texture(ourTexture1, Vert.texcoord), texture(ourTexture2, Vert.texcoord), 0.7);
}