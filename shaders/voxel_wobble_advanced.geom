#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    float AmbientOcclusion;
} gs_in[];

out GS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    float AmbientOcclusion;
} gs_out;

uniform float time;
uniform float wobbleStrength = 0.2;
uniform float wobbleFrequency = 2.0;

// === VERSCHIEDENE WABBER-MODI ===
uniform int wobbleMode = 0; // 0=Wasser, 1=Lava, 2=Portal, 3=Wind

vec3 waterWobble(vec3 pos) {
    float wave = sin(pos.x * wobbleFrequency + time) * 
       cos(pos.z * wobbleFrequency + time * 0.7);
    wave += sin(pos.x * wobbleFrequency * 1.5 + time * 1.3) * 0.5;
    return vec3(0.0, wave * wobbleStrength, 0.0);
}

vec3 lavaWobble(vec3 pos) {
    // Langsamer, dickflüssiger
    float wave1 = sin(pos.x * wobbleFrequency * 0.5 + time * 0.5) * 
           cos(pos.z * wobbleFrequency * 0.5 + time * 0.3);
    float wave2 = sin(pos.x * wobbleFrequency * 0.3 + time * 0.4) * 0.7;
    return vec3(0.0, (wave1 + wave2) * wobbleStrength * 1.5, 0.0);
}

vec3 portalWobble(vec3 pos) {
 // Spirale
    float angle = time + length(pos.xz) * wobbleFrequency;
    float radius = wobbleStrength * sin(time * 2.0);
    return vec3(cos(angle) * radius, 0.0, sin(angle) * radius);
}

vec3 windWobble(vec3 pos) {
    // Nur X-Richtung, abhängig von Y-Position (oben bewegt sich mehr)
    float heightFactor = max(0.0, pos.y / 10.0); // Annahme: Gras bis Y=10
    float sway = sin(pos.x * wobbleFrequency + time) * heightFactor;
    return vec3(sway * wobbleStrength, 0.0, 0.0);
}

vec3 calculateWobble(vec3 position) {
    if (wobbleMode == 0) return waterWobble(position);
    if (wobbleMode == 1) return lavaWobble(position);
    if (wobbleMode == 2) return portalWobble(position);
    if (wobbleMode == 3) return windWobble(position);
    return vec3(0.0);
}

void main() {
    vec3 center = (gl_in[0].gl_Position.xyz + 
     gl_in[1].gl_Position.xyz + 
       gl_in[2].gl_Position.xyz) / 3.0;
    
    vec3 wobbleOffset = calculateWobble(center);
    
    for(int i = 0; i < 3; i++) {
   vec4 wobbledPos = gl_in[i].gl_Position + vec4(wobbleOffset, 0.0);
        gl_Position = wobbledPos;
        
   gs_out.FragPos = gs_in[i].FragPos + wobbleOffset;
  gs_out.Normal = gs_in[i].Normal;
        gs_out.TexCoords = gs_in[i].TexCoords;
        gs_out.AmbientOcclusion = gs_in[i].AmbientOcclusion;
   
        EmitVertex();
    }
    
    EndPrimitive();
}
