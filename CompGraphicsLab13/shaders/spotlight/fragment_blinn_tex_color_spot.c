#version 330 core

uniform struct Spotlight {
	vec4 position;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec3 attenuation;
	vec3 spotdirection;
	float spotcutoff;
	float spotexponent;
} spot;

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
void main() {
	vec3 normal = normalize(Vert.normal);
	vec3 lightDir = normalize(Vert.lightDir);
	vec3 viewDir = normalize(Vert.viewDir);

	vec3 H = normalize(Vert.lightDir + Vert.viewDir);
	float n = 16;

	float spotEffect = dot(normalize(spot.spotdirection), -lightDir);
	float spot_limit = float(spotEffect > spot.spotcutoff);
	spotEffect = max(pow(spotEffect, spot.spotexponent), 0.0);
	float attenuation = spot_limit*spotEffect/(spot.attenuation[0] + spot.attenuation[1] * Vert.distance + spot.attenuation[2] * Vert.distance * Vert.distance); 	

	vec4 color = material.emission;
	color += material.ambient * spot.ambient * attenuation;
	float Ndot = max(dot(normal,lightDir),0.0);
	color += material.diffuse * spot.diffuse * Ndot* attenuation;

	float RdotVpow = max(pow(dot(reflect (-lightDir, normal), viewDir), material.shininess),0.0);
	color += material.specular * spot.specular * RdotVpow * attenuation * pow(dot(normal, H), n);
		
	gl_FragColor = color * material.color_obj * texture(ourTexture, Vert.texcoord);
}