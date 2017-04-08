#version 330 core

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
}; 

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 planeNormal;
in vec3 fragPos; 

out vec4 color;

uniform vec3 viewPos;
uniform Light light;
uniform Material material;

void main()
{
	//ambient
    vec3 ambient = material.ambient * light.ambient;

	vec3 norm = normalize(planeNormal);

	//diffuse
	vec3 lightDir = normalize(light.position - fragPos);  
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * material.diffuse * light.diffuse;

	//specular
	vec3 viewDir = normalize(viewPos - fragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 specular = spec * material.specular * light.specular;   

    vec3 result = ambient + diffuse + specular;
    color = vec4(result, 1.0f);
}