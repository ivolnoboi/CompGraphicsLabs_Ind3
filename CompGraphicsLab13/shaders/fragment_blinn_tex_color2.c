#version 330 core

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
	vec3 lightDir2;
	vec3 viewDir;
	float distance;
	float distance2;
} Vert;
uniform sampler2D ourTexture;
void main() {
	vec3 normal = normalize(Vert.normal);
	vec3 lightDir = normalize(Vert.lightDir);
	vec3 viewDir = normalize(Vert.viewDir);

	vec3 H = normalize(Vert.lightDir + Vert.viewDir);
	float n = 16;

	float attenuation = 1.0/(light.attenuation[0] + light.attenuation[1] * Vert.distance + light.attenuation[2] * Vert.distance * Vert.distance); 	

	vec4 color = material.emission;
	color += material.ambient * light.ambient * attenuation;
	float Ndot = max(dot(normal,lightDir),0.0);
	color += material.diffuse * light.diffuse * Ndot* attenuation;

	float RdotVpow = max(pow(dot(reflect (-lightDir, normal), viewDir), material.shininess),0.0);
	color += material.specular * light.specular * RdotVpow * attenuation * pow(dot(normal, H), n);


	vec3 lightDir2 = normalize(Vert.lightDir2);

	vec3 H2 = normalize(Vert.lightDir2 + Vert.viewDir);

	float attenuation2 = 1.0 / (light2.attenuation[0] + light2.attenuation[1] * Vert.distance2 + light2.attenuation[2] * Vert.distance2 * Vert.distance2);

	color += material.ambient * light2.ambient * attenuation2;
	float Ndot2 = max(dot(normal, lightDir2), 0.0);
	color += material.diffuse * light2.diffuse * Ndot2 * attenuation2;

	float RdotVpow2 = max(pow(dot(reflect(-lightDir2, normal), viewDir), material.shininess), 0.0);
	color += material.specular * light2.specular * RdotVpow2 * attenuation2 * pow(dot(normal, H2), n);
		
	gl_FragColor = color * material.color_obj * texture(ourTexture, Vert.texcoord);
}