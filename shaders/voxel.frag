#version 330 core
out vec4 FragColor;

in VS_OUT {
 vec3 FragPos;
    vec3 Normal;
  vec2 TexCoords;
    float AmbientOcclusion;
} fs_in;

uniform sampler2D diffuseTexture;  // Textur für Blöcke
uniform vec3 sunDirection;    // Richtung der Sonne (normalisiert)
uniform vec3 sunColor;       // Farbe der Sonne
uniform vec3 ambientColor; // Ambiente Beleuchtung
uniform vec3 viewPos;       // Kamera-Position

void main()
{       
    // Lade Block-Farbe aus Textur
    vec3 blockColor = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
 
    // === 1. AMBIENTE BELEUCHTUNG ===
    vec3 ambient = ambientColor * blockColor;
  
    // === 2. DIFFUSE BELEUCHTUNG (Sonne) ===
    // Sonne scheint von oben/vorne
    float diff = max(dot(normal, -sunDirection), 0.0);
    vec3 diffuse = diff * sunColor * blockColor;
 
    // === 3. SIMPLE AMBIENT OCCLUSION ===
    // Faces die nach unten zeigen sind dunkler
    float ao = fs_in.AmbientOcclusion;
 
    // === 4. EINFACHES FOG ===
    float distance = length(viewPos - fs_in.FragPos);
    float fogFactor = clamp(1.0 - (distance - 20.0) / 80.0, 0.0, 1.0);
    vec3 fogColor = vec3(0.5, 0.5, 0.8); // Himmel-Farbe
    
    // === 5. FINALE BELEUCHTUNG ===
    vec3 lighting = (ambient + diffuse) * ao;
    
    // Fog anwenden
    vec3 finalColor = mix(fogColor, lighting, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}
