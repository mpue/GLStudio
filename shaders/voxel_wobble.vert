#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// Output für Geometry Shader
out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    float AmbientOcclusion;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    // Berechne Fragment-Position (vor Wabber)
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.Normal = transpose(inverse(mat3(model))) * aNormal;
    vs_out.TexCoords = aTexCoords;
 
    // Einfache Ambient Occlusion basierend auf Normale
    vs_out.AmbientOcclusion = 0.7 + 0.3 * dot(aNormal, vec3(0.0, 1.0, 0.0));
  
  // Position OHNE Projection/View - das macht der Geometry Shader
    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}
