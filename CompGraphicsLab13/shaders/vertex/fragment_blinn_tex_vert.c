#version 330 core

in struct Vertex {
	vec2 texcoord;
	vec3 normal;
	vec3 lightDir;
	vec3 viewDir;
	float distance;
} Vert;
uniform sampler2D ourTexture;
in vec4 var_color;
void main() 
{
	gl_FragColor = var_color * texture(ourTexture, Vert.texcoord);
}