#version 330 core

in vec3 position;
in vec2 texcoord;
in vec3 normal;
uniform struct Transform {
	mat4 model;
	mat4 viewProjection;
	mat3 normal;
	vec3 viewPosition;
} transform;

uniform struct PointLight{
	vec4 position;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec3 attenuation;
} light;

uniform struct PointLight {
	vec4 position;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec3 attenuation;
} light2;

out struct Vertex {
	vec2 texcoord;
	vec3 normal;
	vec3 lightDir;
	vec3 lightDir2;
	vec3 viewDir;
	float distance;
	float distance2;
} Vert;

void main() {
	vec4 vertex = transform.model * vec4(position, 1.0);
	vec4 lightDir = light.position - vertex;
	vec4 lightDir2 = light2.position - vertex;
	gl_Position = transform.viewProjection * vertex;
	Vert.texcoord = texcoord;
	Vert.normal = transform.normal * normal;
	Vert.lightDir = vec3(lightDir);
	Vert.lightDir2 = vec3(lightDir2);
	Vert.viewDir = transform.viewPosition -vec3(vertex);
	Vert.distance = length(lightDir);
	Vert.distance2 = length(lightDir2);
}