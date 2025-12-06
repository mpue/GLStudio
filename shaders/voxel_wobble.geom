#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

// Input vom Vertex Shader
in VS_OUT {
    vec3 FragPos;
  vec3 Normal;
    vec2 TexCoords;
    float AmbientOcclusion;
} gs_in[];

// Output zum Fragment Shader
out GS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    float AmbientOcclusion;
} gs_out;

// Uniforms für den Wabber-Effekt
uniform float time;
uniform float wobbleStrength = 0.2;    // Stärke der Wabber-Bewegung
uniform float wobbleFrequency = 2.0;   // Frequenz der Wellen
uniform vec3 wobbleDirection = vec3(0.0, 1.0, 0.0); // Richtung (Y-Achse für Wasser)

// Berechne Wabber-Offset basierend auf Position und Zeit
vec3 calculateWobble(vec3 position) {
    // Verwende Sinus-Welle für sanfte Bewegung
    float wave = sin(position.x * wobbleFrequency + time) * 
   cos(position.z * wobbleFrequency + time * 0.7);
    
    // Optional: Füge zusätzliche Welle für komplexere Bewegung hinzu
    wave += sin(position.x * wobbleFrequency * 1.5 + time * 1.3) * 0.5;
    wave += cos(position.z * wobbleFrequency * 0.8 + time * 0.9) * 0.5;
    
    return wobbleDirection * wave * wobbleStrength;
}

void main() {
    // Berechne Zentrum des Dreiecks
    vec3 center = (gl_in[0].gl_Position.xyz + 
     gl_in[1].gl_Position.xyz + 
            gl_in[2].gl_Position.xyz) / 3.0;
    
    // Berechne Wabber-Offset für dieses Dreieck
    vec3 wobbleOffset = calculateWobble(center);
    
    // Gebe alle 3 Vertices des Dreiecks mit Wabber-Effekt aus
    for(int i = 0; i < 3; i++) {
        // Wende Wabber auf Vertex-Position an
        vec4 wobbledPos = gl_in[i].gl_Position + vec4(wobbleOffset, 0.0);
        gl_Position = wobbledPos;
     
        // Übergebe Attribute an Fragment Shader
        gs_out.FragPos = gs_in[i].FragPos + wobbleOffset;
        gs_out.Normal = gs_in[i].Normal;
        gs_out.TexCoords = gs_in[i].TexCoords;
gs_out.AmbientOcclusion = gs_in[i].AmbientOcclusion;
        
        EmitVertex();
    }
    
    EndPrimitive();
}
