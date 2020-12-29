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

uniform struct Material {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 emission;
	float shininess;
	vec4 color_obj;
} material;

out struct Vertex {
	vec2 texcoord;
	vec3 normal;
	vec3 lightDir;
	vec3 viewDir;
	float distance;
} Vert;
out vec4 var_color;

void main() {
	vec4 vertex = transform.model * vec4(position, 1.0);
	vec4 lightDir = light.position - vertex;
	vec4 lightDir2 = light2.position - vertex;
	gl_Position = transform.viewProjection * vertex;
	Vert.texcoord = texcoord;
	Vert.normal = transform.normal * normal;
	Vert.lightDir = vec3(lightDir);
	vec3 vert_lightDir2 = vec3(lightDir2);
	Vert.viewDir = transform.viewPosition -vec3(vertex);
	Vert.distance = length(lightDir);
	float vert_distance2 = length(lightDir2);

	vec3 normal_frag = normalize(Vert.normal);
	vec3 lightDir_frag = normalize(Vert.lightDir);
	vec3 lightDir_frag2 = normalize(vert_lightDir2);
	vec3 viewDir = normalize(Vert.viewDir);

	vec3 H = normalize(Vert.lightDir + Vert.viewDir);
	vec3 H2 = normalize(vert_lightDir2 + Vert.viewDir);
	float n = 16;

	float attenuation = 1.0 / (light.attenuation[0] + light.attenuation[1] * Vert.distance + light.attenuation[2] * Vert.distance * Vert.distance);
	float attenuation2 = 1.0 / (light2.attenuation[0] + light2.attenuation[1] * vert_distance2 + light2.attenuation[2] * vert_distance2 * vert_distance2);

	vec4 color = material.emission;
	color += material.ambient * light.ambient * attenuation;
	color += material.ambient * light2.ambient * attenuation2;
	float Ndot = max(dot(normal_frag, lightDir_frag), 0.0);
	float Ndot2 = max(dot(normal_frag, lightDir_frag2), 0.0);
	color += material.diffuse * light.diffuse * Ndot * attenuation;
	color += material.diffuse * light2.diffuse * Ndot2 * attenuation2;

	float RdotVpow = max(pow(dot(reflect(-lightDir_frag, normal_frag), viewDir), material.shininess), 0.0);
	float RdotVpow2 = max(pow(dot(reflect(-lightDir_frag2, normal_frag), viewDir), material.shininess), 0.0);
	color += material.specular * light.specular * RdotVpow * attenuation * pow(dot(normal_frag, H), n);
	color += material.specular * light2.specular * RdotVpow2 * attenuation2 * pow(dot(normal_frag, H2), n);

	var_color = color;
}