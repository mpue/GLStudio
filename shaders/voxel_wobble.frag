#version 330 core
out vec4 FragColor;

// Input vom Geometry Shader
in GS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
 float AmbientOcclusion;
} fs_in;

uniform sampler2D textureAtlas;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

void main()
{
    // Hol Textur-Farbe
    vec4 texColor = texture(textureAtlas, fs_in.TexCoords);
    
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor * fs_in.AmbientOcclusion;
  
    // Diffuse
    vec3 norm = normalize(fs_in.Normal);
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
  float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular (optional, für Wasser-Glanz)
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

 // Kombiniere Beleuchtung
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    
    FragColor = vec4(result, texColor.a);
}
