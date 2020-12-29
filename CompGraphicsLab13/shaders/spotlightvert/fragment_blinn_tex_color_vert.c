#version 330 core

uniform struct Material{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 emission;
	float shininess;
	vec4 color_obj;
} material;

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
	gl_FragColor = var_color * material.color_obj * texture(ourTexture, Vert.texcoord);
}