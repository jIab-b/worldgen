varying vec3 vNormal;
 
void main() {
    vec3 light = normalize(vec3(0.5, 1.0, 0.75));
    float diffuse = max(0.0, dot(vNormal, light));
    gl_FragColor = vec4(vec3(diffuse), 1.0);
} 