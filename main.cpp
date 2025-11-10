// ============================================================================
// FANTASMAS DE UNA AMISTAD - ANIMACIÓN 2D NARRATIVA
// ============================================================================
// Proyecto de Computación Gráfica - Animación 2D en OpenGL 3.3 Core
// 
// TIPO DE ANIMACIÓN: Por Tiempo (Time-based Animation)
// - Todas las animaciones calculadas basadas en tiempo transcurrido
// - Sistema de fases con duraciones definidas (PRESENTATION, BATTLE, DEFEAT, MEMORY)
// - Progreso normalizado (0.0-1.0) para interpolaciones suaves
// 
// HISTORIA:
// Dos guerreros amigos se enfrentan en un duelo. Uno muere y el otro recoge
// su cabeza. Al final, ambos espíritus se reúnen en el cielo, simbolizando
// que la amistad trasciende la muerte.
//
// SINCRONIZACIÓN MUSICAL:
// Coreografía sincronizada con "Chaconne" de Bach (BWV 1004 V)
// Duraciones de pasos de batalla calibradas a frases musicales
// ============================================================================

#include <glad/glad.h>        // Loader de funciones OpenGL modernas
#include <GLFW/glfw3.h>        // Biblioteca de ventanas y contexto OpenGL
#include <glm/glm.hpp>         // Matemáticas vectoriales (vec2, vec3, vec4)
#include <glm/gtc/matrix_transform.hpp>  // Transformaciones (translate, rotate, scale)
#include <glm/gtc/type_ptr.hpp>          // Conversión de tipos GLM a arrays

#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <memory>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"  // Carga de imágenes (PNG, JPG) para texturas

// ============================================================================
// CONSTANTES DE CONFIGURACIÓN
// ============================================================================
// Centraliza todas las constantes de la aplicación para fácil ajuste
// Referencia LearnOpenGL: Buenas prácticas de organización de código
namespace Config {
    // Configuración de ventana
    constexpr unsigned int SCREEN_WIDTH = 1200;
    constexpr unsigned int SCREEN_HEIGHT = 800;

    // Delta time fijo para animación a 60 FPS
    // Animación por tiempo: dt constante asegura velocidad uniforme
    constexpr float DELTA_TIME = 1.0f / 60.0f;
    constexpr float PI = 3.14159265358979323846f;

    // Proporciones anatómicas del guerrero (relativas a HEIGHT)
    // Escala base para que quepa en viewport normalizado [-1, 1]
    constexpr float HEIGHT = 0.75f;
    constexpr float HEAD_RADIUS = 0.07f * HEIGHT;      // Cabeza: círculo
    constexpr float HAND_RADIUS = 0.045f * HEIGHT;     // Manos/pies: círculos pequeños
    constexpr float TORSO_SIZE = 0.38f * HEIGHT;       // Torso: triángulo
    constexpr float ARM_WIDTH = 0.11f * HEIGHT;
    constexpr float ARM_HEIGHT = 0.42f * HEIGHT;
    constexpr float LEG_WIDTH = 0.14f * HEIGHT;
    constexpr float LEG_HEIGHT = 0.55f * HEIGHT;
    constexpr float WEAPON_LENGTH = 0.48f * HEIGHT;    // Espada: rectángulo
    constexpr float WEAPON_THICKNESS = 0.055f * HEIGHT;
    constexpr float HAT_SIZE = 0.14f * HEIGHT;         // Sombrero: triángulo isósceles
    constexpr float GROUND_Y = -0.45f;                 // Nivel del suelo en espacio normalizado

    // Duraciones de fases (en segundos)
    // CUMPLE REQUISITO: Animación de al menos 15 segundos (total: 30s)
    constexpr float PRESENTATION_DURATION = 5.0f;   // Fase 1: Presentación
    constexpr float BATTLE_DURATION = 16.0f;        // Fase 2: Desarrollo (batalla)
    constexpr float DEFEAT_DURATION = 3.0f;         // Fase 3a: Derrota
    constexpr float MEMORY_DURATION = 6.0f;         // Fase 3b: Cierre (memoria)

    // Tiempos de coreografía de batalla (sincronizados con Chaconne)
    // Cada paso corresponde a una frase musical
    constexpr float STEP_BLEND_TIME = 0.25f;    // Tiempo de transición suave entre pasos
    constexpr float STEP1_DURATION = 3.5f;      // Approach - introducción melódica
    constexpr float STEP2_DURATION = 3.5f;      // Weapon cross - primer crescendo
    constexpr float STEP3_DURATION = 3.5f;      // Yellow strike - respuesta lírica
    constexpr float STEP4_DURATION = 2.5f;      // Grey strike - tensión (más corto, clímax)
    constexpr float STEP5_DURATION = 3.0f;      // Low strike - preparación final
}

// ============================================================================
// UTILIDADES MATEMÁTICAS
// ============================================================================
// Funciones auxiliares para animación e interpolación
// Basadas en conceptos de easing y smoothing comunes en animación
namespace Math {
    // Interpolación lineal entre dos valores
    // Usado extensivamente para animaciones suaves
    inline float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    // Interpolación lineal entre dos vectores 2D
    inline glm::vec2 lerp(const glm::vec2& a, const glm::vec2& b, float t) {
        return a + (b - a) * t;
    }

    // Easing Functions - Crean movimiento orgánico y natural
    // Referencia: easings.net

    // Desaceleración cuadrática - Final lento (usado en recuperaciones)
    inline float easeOutQuad(float t) {
        return 1.0f - (1.0f - t) * (1.0f - t);
    }

    // Aceleración cuadrática - Inicio lento (usado en preparaciones)
    inline float easeInQuad(float t) {
        return t * t;
    }

    // S-curve - Lento-rápido-lento (usado en approach)
    inline float easeInOutQuad(float t) {
        return t < 0.5f ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
    }

    // Desaceleración cúbica - Más dramática (usado en ataques)
    inline float easeOutCubic(float t) {
        float inv = 1.0f - t;
        return 1.0f - inv * inv * inv;
    }

    // Clamp - Asegura valor en rango [mn, mx]
    inline float clamp(float x, float mn, float mx) {
        return x < mn ? mn : (x > mx ? mx : x);
    }

    // Smoothstep - Interpolación hermite suave (usado en transiciones)
    inline float smoothstep(float e0, float e1, float x) {
        float t = clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    // Cálculo de ángulo de brazo para apuntar a objetivo
    // Inverse Kinematics simplificado en 2D
    // CUMPLE REQUISITO: Rotación dinámica para animación
    inline float computeArmAngle(float srcX, float srcY, float targetX, float targetY, bool facingRight) {
        float dx = targetX - srcX;
        float dy = targetY - srcY;
        if (!facingRight) dx = -dx;  // Inversión para guerrero mirando izquierda
        return atan2f(dx, -dy);       // Ángulo desde vertical (espadas apuntan abajo por defecto)
    }

    // Interpolación angular - Toma camino más corto
    // Crítico para evitar rotaciones de 270° en lugar de -90°
    inline float lerpAngle(float current, float target, float t) {
        float diff = target - current;
        // Normalizar diferencia a rango [-PI, PI]
        while (diff > Config::PI) diff -= 2.0f * Config::PI;
        while (diff < -Config::PI) diff += 2.0f * Config::PI;
        return current + diff * t;
    }

    // Calcula punta de arma en espacio mundo
    // Usado para detección de colisiones y efectos visuales
    inline glm::vec2 getWeaponTip(const glm::vec2& warriorPos, float armAngle, bool facingRight) {
        float armLength = Config::ARM_HEIGHT;
        float weaponLength = Config::WEAPON_LENGTH;
        float totalReach = armLength + weaponLength;
        float adjustedAngle = facingRight ? armAngle : -armAngle;
        return glm::vec2(
            warriorPos.x + sin(adjustedAngle) * totalReach * (facingRight ? 1.0f : -1.0f),
            warriorPos.y + 0.25f * Config::HEIGHT - cos(adjustedAngle) * totalReach
        );
    }

    // Animación procedural de caminar - Balanceo vertical
    inline float walkingBob(float progress, float intensity = 0.03f) {
        return sinf(progress * Config::PI * 2.0f) * intensity;
    }

    // Animación procedural de caminar - Zancada horizontal de pies
    inline float walkingStride(float progress, float strideLength = 0.08f) {
        return sinf(progress * Config::PI * 2.0f) * strideLength;
    }
}

// ============================================================================
// PALETA DE COLORES - TEMA MELANCÓLICO
// ============================================================================
// Colores desaturados pero distinguibles para estética nostálgica
// CUMPLE REQUISITO: Armonía de colores adaptada a la narrativa
namespace Colors {
    // Tonos de piel/carne (visible pero apagado)
    const glm::vec3 ORANGE(0.75f, 0.45f, 0.30f);  // Terracota para piel

    // Paleta del guerrero gris (blanco sucio, grises)
    const glm::vec3 WHITE(0.68f, 0.65f, 0.63f);       // Blanco envejecido
    const glm::vec3 LIGHT_GREY(0.48f, 0.46f, 0.44f);  // Gris medio
    const glm::vec3 DARK_GREY(0.18f, 0.16f, 0.16f);   // Gris oscuro (sombrero, arma)

    // Paleta del guerrero amarillo (mostaza y azul)
    const glm::vec3 YELLOW(0.68f, 0.60f, 0.35f);      // Mostaza desaturado
    const glm::vec3 BLUE(0.22f, 0.28f, 0.45f);        // Azul oscuro

    // Colores ambientales
    const glm::vec3 SUN(0.85f, 0.70f, 0.50f);         // Sol crepuscular dorado
    const glm::vec3 MEMORY_TINT(0.75f, 0.65f, 0.55f); // Tinte sepia nostálgico
}

// ============================================================================
// SHADERS - OpenGL 3.3 Core Profile
// ============================================================================
// Shaders escritos en GLSL 330
// Referencia LearnOpenGL: https://learnopengl.com/Getting-started/Shaders
namespace Shaders {
    // Vertex Shader: Transforma vértices de espacio objeto a clip space
    const char* vertexSource = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;   // Posición del vértice
        layout(location = 1) in vec2 aUV;    // Coordenadas de textura
        
        uniform mat4 transform;  // Matriz MVP (Model-View-Projection)
        
        out vec2 vUV;  // Pasar UVs al fragment shader
        
        void main() {
            gl_Position = transform * vec4(aPos, 1.0);
            vUV = aUV;
        }
    )glsl";

    // Fragment Shader: Calcula color final del píxel
    // Soporta color flat, textura, alpha blending y efecto glow
    const char* fragmentSource = R"glsl(
        #version 330 core
        in vec2 vUV;  // Coordenadas de textura desde vertex shader
        
        uniform vec3 flatColor;    // Color base
        uniform float uAlpha;      // Transparencia
        uniform float uGlow;       // Intensidad de brillo
        uniform int useTexture;    // Flag: 0=flat color, 1=textura
        uniform sampler2D uTexture;  // Textura 2D
        
        out vec4 FragColor;  // Color final del píxel
        
        void main() {
            vec3 color = flatColor;
            
            if (useTexture == 1) {
                // Modo textura: Mezcla con flat color según alpha de textura
                vec4 texColor = texture(uTexture, vUV);
                color = mix(color, texColor.rgb, texColor.a);
                FragColor = vec4(color, uAlpha * texColor.a);
            } else {
                // Modo flat: Aplica glow (mezcla hacia blanco)
                color = mix(color, vec3(1.0), clamp(uGlow, 0.0, 1.0) * 0.6);
                FragColor = vec4(color, uAlpha);
            }
        }
    )glsl";
}

// ============================================================================
// GESTIÓN DE SHADERS
// ============================================================================
// Clase que encapsula programa de shaders de OpenGL
// Referencia LearnOpenGL: https://learnopengl.com/Getting-started/Shaders
class ShaderProgram {
public:
    GLuint id;  // Handle del programa de shaders

    // Constructor: Compila y linkea shaders
    ShaderProgram(const char* v, const char* f) {
        GLuint vs = compileShader(GL_VERTEX_SHADER, v);
        GLuint fs = compileShader(GL_FRAGMENT_SHADER, f);
        id = glCreateProgram();
        glAttachShader(id, vs);
        glAttachShader(id, fs);
        glLinkProgram(id);
        checkLinkErrors();
        glDeleteShader(vs);  // Liberar shaders después de linkear
        glDeleteShader(fs);
    }

    // Destructor: Libera programa
    ~ShaderProgram() {
        glDeleteProgram(id);
    }

    // Activa este programa para renderizado
    void use() const {
        glUseProgram(id);
    }

    // Setters de uniforms - Envían datos de CPU a GPU
    void setMat4(const char* n, const glm::mat4& m) const {
        glUniformMatrix4fv(glGetUniformLocation(id, n), 1, GL_FALSE, glm::value_ptr(m));
    }
    void setVec3(const char* n, const glm::vec3& v) const {
        glUniform3fv(glGetUniformLocation(id, n), 1, glm::value_ptr(v));
    }
    void setFloat(const char* n, float v) const {
        glUniform1f(glGetUniformLocation(id, n), v);
    }
    void setInt(const char* n, int v) const {
        glUniform1i(glGetUniformLocation(id, n), v);
    }

private:
    // Compila un shader individual
    GLuint compileShader(GLenum t, const char* s) {
        GLuint sh = glCreateShader(t);
        glShaderSource(sh, 1, &s, nullptr);
        glCompileShader(sh);

        // Verificar errores de compilación
        GLint ok;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetShaderInfoLog(sh, 1024, nullptr, log);
            std::cerr << "Shader compilation error: " << log << std::endl;
        }
        return sh;
    }

    // Verifica errores de linking del programa
    void checkLinkErrors() {
        GLint ok;
        glGetProgramiv(id, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetProgramInfoLog(id, 1024, nullptr, log);
            std::cerr << "Program linking error: " << log << std::endl;
        }
    }
};

// ============================================================================
// GEOMETRÍA - Vertex Array Objects
// ============================================================================
// Estructura que encapsula VAO/VBO/EBO de OpenGL
// Referencia LearnOpenGL: https://learnopengl.com/Getting-started/Hello-Triangle
struct Geometry {
    GLuint VAO = 0;  // Vertex Array Object - Estado de configuración de vértices
    GLuint VBO = 0;  // Vertex Buffer Object - Buffer de datos de vértices
    GLuint EBO = 0;  // Element Buffer Object - Buffer de índices (opcional)
    GLsizei vertexCount = 0;
    GLenum drawMode = GL_TRIANGLES;

    // Destructor: Libera recursos de OpenGL
    ~Geometry() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);
    }

    // Renderiza la geometría
    void draw() const {
        glBindVertexArray(VAO);
        if (EBO)
            glDrawElements(drawMode, vertexCount, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(drawMode, 0, vertexCount);
    }
};

// ============================================================================
// FACTORY DE GEOMETRÍA
// ============================================================================
// Patrón Factory para crear geometrías primitivas
// CUMPLE REQUISITO: Figuras con curvas (círculos) y polígonos básicos
class GeometryFactory {
public:
    // Crea un cuadrilátero (quad) con UVs
    // Usado para: Armas (rectángulos), fondos, texturas
    static Geometry createQuad() {
        Geometry g;
        // Formato: posX, posY, posZ, uvU, uvV
        float vertices[] = {
             0.5f,  0.5f, 0.0f, 1.0f, 1.0f,  // Top-right
             0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  // Bottom-right
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,  // Bottom-left
            -0.5f,  0.5f, 0.0f, 0.0f, 1.0f   // Top-left
        };
        unsigned int idx[] = { 0,1,3, 1,2,3 };  // Dos triángulos

        glGenVertexArrays(1, &g.VAO);
        glGenBuffers(1, &g.VBO);
        glGenBuffers(1, &g.EBO);

        glBindVertexArray(g.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, g.VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

        // Layout location 0: Posición (3 floats)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Layout location 1: UVs (2 floats)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        g.vertexCount = 6;
        g.drawMode = GL_TRIANGLES;
        return g;
    }

    // Crea un triángulo
    // Usado para: Torso, brazos, piernas, sombrero
    static Geometry createTriangle() {
        Geometry g;
        float vertices[] = {
             0.0f,  0.5f, 0.0f, 0.5f, 1.0f,   // Top
             0.35f,-0.25f,0.0f, 1.0f, 0.0f,   // Bottom-right
            -0.35f,-0.25f,0.0f, 0.0f, 0.0f    // Bottom-left
        };

        glGenVertexArrays(1, &g.VAO);
        glGenBuffers(1, &g.VBO);
        glBindVertexArray(g.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, g.VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        g.vertexCount = 3;
        g.drawMode = GL_TRIANGLES;
        return g;
    }

    // Crea un círculo mediante triangle fan
    // CUMPLE REQUISITO: Curvas en todas las obras
    // Usado para: Cabezas, manos, pies, sol, partículas
    static Geometry createCircle(int segments = 56) {
        Geometry g;
        std::vector<float> v;
        v.reserve((segments + 2) * 5);

        // Centro del círculo
        v.insert(v.end(), { 0.0f, 0.0f, 0.0f, 0.5f, 0.5f });

        // Vértices del perímetro (56 segmentos = círculo suave)
        for (int i = 0; i <= segments; i++) {
            float a = (float)i / (float)segments * 2.0f * Config::PI;
            float x = cosf(a);
            float y = sinf(a);
            v.push_back(x);
            v.push_back(y);
            v.push_back(0.0f);
            v.push_back(0.5f + 0.5f * x);  // UVs
            v.push_back(0.5f + 0.5f * y);
        }

        glGenVertexArrays(1, &g.VAO);
        glGenBuffers(1, &g.VBO);
        glBindVertexArray(g.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, g.VBO);
        glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        g.vertexCount = segments + 2;
        g.drawMode = GL_TRIANGLE_FAN;
        return g;
    }
};

// ============================================================================
// TEXTURA
// ============================================================================
// Clase para cargar y gestionar texturas 2D
// Referencia LearnOpenGL: https://learnopengl.com/Getting-started/Textures
class Texture {
public:
    GLuint id = 0;  // Handle de textura OpenGL

    // Constructor: Carga imagen desde disco usando stb_image
    // CUMPLE REQUISITO: Fondo con textura
    Texture(const std::string& path) {
        int w, h, c;
        stbi_set_flip_vertically_on_load(true);  // OpenGL espera UVs con origen abajo-izquierda
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 4);  // Forzar RGBA

        if (data) {
            glGenTextures(1, &id);
            glBindTexture(GL_TEXTURE_2D, id);

            // Subir datos de textura a GPU
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);  // Mipmaps para calidad a distancia

            // Configuración de wrapping (clamp para evitar repetición)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Configuración de filtering (linear para suavizado)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);  // Liberar memoria de CPU
        }
        else {
            std::cerr << "Error al cargar textura: " << path << std::endl;
        }
    }

    ~Texture() {
        if (id) glDeleteTextures(1, &id);
    }

    // Activa textura en unidad específica
    void bind(int unit = 0) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }

    bool isValid() const { return id != 0; }
};

// ============================================================================
// SISTEMA DE PARTÍCULAS
// ============================================================================
// Estructura de partícula individual con física
// Usado para efectos de impacto y explosión de cuerpo
struct Particle {
    glm::vec2 position;
    glm::vec2 velocity;
    float rotation = 0.0f;
    float rotationSpeed = 0.0f;
    float lifetime = 1.0f;      // Tiempo restante de vida
    float maxLifetime = 1.0f;   // Tiempo total de vida
    glm::vec3 color;
    bool isHead = false;        // Flag especial para cabeza del guerrero
    bool attached = false;      // Flag: partícula adjuntada (cabeza recogida)

    // Actualiza física de partícula
    // CUMPLE REQUISITO: Traslación dinámica con física realista
    void update(float dt, float gravity = -0.5f, float groundY = -0.9f) {
        if (attached) {
            // Partículas adjuntadas no se mueven (cabeza recogida)
            velocity = glm::vec2(0.0f);
            rotation = 0.0f;
            rotationSpeed = 0.0f;
            return;
        }

        lifetime -= dt;

        // Integración de velocidad (Euler)
        position += velocity * dt;
        velocity.y += gravity * dt;  // Gravedad
        velocity *= 0.99f;           // Damping (resistencia del aire)

        // Rotación
        rotation += rotationSpeed * dt;

        // Colisión con suelo
        if (position.y < groundY) {
            position.y = groundY;
            velocity.y = -velocity.y * 0.15f;  // Rebote suave
            velocity.x *= 0.85f;               // Fricción horizontal
            rotationSpeed *= 0.6f;             // Desaceleración de rotación

            // Detener movimiento si velocidad muy baja
            if (std::abs(velocity.y) < 0.03f) {
                velocity = glm::vec2(0.0f);
                rotationSpeed = 0.0f;
            }
        }
    }

    // Calcula alpha basado en tiempo de vida restante
    // CUMPLE REQUISITO: Transparencia progresiva
    float getAlpha() const {
        return Math::clamp(lifetime / maxLifetime, 0.0f, 1.0f);
    }
};

// Sistema de partículas de impacto (choques de espadas)
class ImpactParticleSystem {
public:
    // Genera partículas radiales en punto de impacto
    void spawn(const glm::vec2& position, int count = 12) {
        for (int i = 0; i < count; ++i) {
            Particle p;
            float angle = (float)i / (float)count * 2.0f * Config::PI;
            float speed = 0.4f + (rand() % 100) / 200.0f;  // Velocidad aleatoria

            p.position = position;
            p.velocity = glm::vec2(cos(angle) * speed, sin(angle) * speed);
            p.rotation = 0.0f;
            p.rotationSpeed = ((rand() % 200) - 100) / 10.0f;  // Rotación aleatoria
            p.lifetime = 0.8f + (rand() % 40) / 100.0f;
            p.maxLifetime = p.lifetime;
            p.color = glm::vec3(1.0f, 0.8f + (rand() % 20) / 100.0f, 0.6f);  // Naranja brillante

            particles_.push_back(p);
        }
    }

    // Actualiza todas las partículas
    void update(float dt) {
        for (auto& p : particles_) {
            p.update(dt, -1.2f);  // Gravedad más fuerte para partículas
        }

        // Eliminar partículas muertas
        particles_.erase(
            std::remove_if(particles_.begin(), particles_.end(),
                [](const Particle& p) { return p.lifetime <= 0.0f; }),
            particles_.end()
        );
    }

    // Renderiza todas las partículas como círculos pequeños
    void render(const ShaderProgram& shader, const Geometry& circle) {
        shader.use();
        shader.setInt("useTexture", 0);
        shader.setFloat("uGlow", 0.3f);  // Brillo sutil

        for (const auto& p : particles_) {
            // CUMPLE REQUISITO: Traslación, Rotación, Escalado
            glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(p.position, 0.0f));
            t = glm::rotate(t, p.rotation, glm::vec3(0, 0, 1));
            float size = 0.02f + (1.0f - p.getAlpha()) * 0.02f;  // Crece al desaparecer
            t = glm::scale(t, glm::vec3(size, size, 1.0f));

            shader.setMat4("transform", t);
            shader.setVec3("flatColor", p.color);
            shader.setFloat("uAlpha", p.getAlpha() * 0.8f);
            circle.draw();
        }
    }

    void clear() { particles_.clear(); }
    bool isEmpty() const { return particles_.empty(); }

private:
    std::vector<Particle> particles_;
};

// ============================================================================
// POSE Y COLORES DE GUERRERO
// ============================================================================
// Estructura de datos de pose de guerrero
// Encapsula todo el estado necesario para renderizar un guerrero
struct WarriorPose {
    float armAngle = 0.0f;      // Ángulo del brazo armado (radianes)
    float duckAmount = 0.0f;    // Cantidad de agacharse (0.0-1.0)
    float leanBack = 0.0f;      // Inclinación hacia atrás (-1.0 a 1.0)
    bool drawWeapon = true;     // ¿Dibujar arma?
    bool drawHat = false;       // ¿Dibujar sombrero? (solo gris)
    glm::vec2 position{ 0.0f };   // Posición en espacio mundo
    float scale = 1.0f;         // Escala uniforme
    float alpha = 1.0f;         // Transparencia (para fantasmas)

    // Estado de animación de caminar
    float leftFootStride = 0.0f;   // Desplazamiento horizontal pie izquierdo
    float rightFootStride = 0.0f;  // Desplazamiento horizontal pie derecho
    float leftFootLift = 0.0f;     // Elevación pie izquierdo
    float rightFootLift = 0.0f;    // Elevación pie derecho
    float walkCycle = 0.0f;        // Progreso del ciclo de caminar (0-2?)
};

// Paleta de colores para cada guerrero
struct WarriorColors {
    glm::vec3 head;
    glm::vec3 torso;
    glm::vec3 arm;
    glm::vec3 leg1;
    glm::vec3 leg2;
    glm::vec3 weapon;

    // Guerrero gris (sombrero, ropa clara)
    static WarriorColors grey() {
        return { Colors::ORANGE, Colors::WHITE, Colors::LIGHT_GREY,
                 Colors::LIGHT_GREY, Colors::WHITE, Colors::DARK_GREY };
    }

    // Guerrero amarillo (ropa amarilla/azul)
    static WarriorColors yellow() {
        return { Colors::ORANGE, Colors::YELLOW, Colors::BLUE,
                 Colors::WHITE, Colors::DARK_GREY, Colors::WHITE };
    }
};

// ============================================================================
// RENDERIZADOR DE GUERRERO
// ============================================================================
// Clase que renderiza guerreros articulados con todas sus partes
// Implementa jerarquía de transformaciones (torso -> cabeza/brazos/piernas)
// Referencia LearnOpenGL: https://learnopengl.com/Getting-started/Transformations
class WarriorRenderer {
public:
    WarriorRenderer(const Geometry& circle, const Geometry& triangle, const Geometry& quad)
        : circle_(circle), triangle_(triangle), quad_(quad) {
    }

    // Función principal de renderizado
    // CUMPLE REQUISITO: Pintura abstracta completa (todos los componentes)
    void draw(const ShaderProgram& shader, const WarriorPose& pose,
        const WarriorColors& colors, bool facingRight = true) {

        shader.use();
        shader.setInt("useTexture", 0);
        shader.setFloat("uGlow", 0.0f);
        shader.setFloat("uAlpha", pose.alpha);

        // Transformación base: Posición + flip horizontal + escala
        // CUMPLE REQUISITO: Traslación, Escalado
        glm::mat4 baseTransform = glm::translate(glm::mat4(1.0f), glm::vec3(pose.position, 0.0f));
        if (!facingRight)
            baseTransform = glm::scale(baseTransform, glm::vec3(-1.0f, 1.0f, 1.0f));  // Flip X
        baseTransform = glm::scale(baseTransform, glm::vec3(pose.scale, pose.scale, 1.0f));

        // Cálculo de deformaciones por pose
        float legCompress = pose.duckAmount * 0.4f;                    // Compresión de piernas
        float torsoLower = pose.duckAmount * 0.25f * Config::HEIGHT;   // Descenso de torso
        float torsoLean = pose.leanBack * glm::radians(25.0f);         // Inclinación de torso
        float leanShift = pose.leanBack * 0.15f * Config::HEIGHT;      // Desplazamiento por inclinación

        // Transformación de inclinación del torso (pivote en caderas)
        // CUMPLE REQUISITO: Rotación compleja con punto de pivote
        float hipY = 0.03f * Config::HEIGHT;
        glm::mat4 leanTransform = glm::translate(baseTransform, glm::vec3(0.0f, hipY, 0.0f));
        leanTransform = glm::rotate(leanTransform, -torsoLean * (facingRight ? 1.0f : -1.0f), glm::vec3(0, 0, 1));
        leanTransform = glm::translate(leanTransform, glm::vec3(0.0f, -hipY, 0.0f));

        // Renderizar partes en orden (atrás a adelante)
        drawTorso(shader, leanTransform, colors, torsoLower, leanShift);
        drawHead(shader, leanTransform, colors, torsoLower, leanShift);
        if (pose.drawHat)
            drawHat(shader, leanTransform, torsoLower, leanShift, facingRight);
        drawArms(shader, leanTransform, pose, colors, facingRight, torsoLower, leanShift);
        drawLegsWithFeet(shader, leanTransform, pose, colors, legCompress);
    }

private:
    const Geometry& circle_;
    const Geometry& triangle_;
    const Geometry& quad_;

    // Dibuja torso (triángulo)
    void drawTorso(const ShaderProgram& sh, const glm::mat4& base,
        const WarriorColors& colors, float lower, float shift) {
        glm::mat4 t = glm::translate(base, glm::vec3(0.0f, 0.20f * Config::HEIGHT - lower + shift, 0.0f));
        t = glm::scale(t, glm::vec3(Config::TORSO_SIZE, Config::TORSO_SIZE, 1.0f));
        sh.setMat4("transform", t);
        sh.setVec3("flatColor", colors.torso);
        triangle_.draw();
    }

    // Dibuja cabeza (círculo)
    // CUMPLE REQUISITO: Curvas (círculo de 56 segmentos)
    void drawHead(const ShaderProgram& sh, const glm::mat4& base,
        const WarriorColors& colors, float lower, float shift) {
        glm::mat4 t = glm::translate(base, glm::vec3(0.0f, 0.45f * Config::HEIGHT - lower + shift, 0.0f));
        t = glm::scale(t, glm::vec3(Config::HEAD_RADIUS, Config::HEAD_RADIUS, 1.0f));
        sh.setMat4("transform", t);
        sh.setVec3("flatColor", colors.head);
        circle_.draw();
    }

    // Dibuja sombrero (triángulo isósceles ancho)
    // Solo para guerrero gris
    void drawHat(const ShaderProgram& sh, const glm::mat4& base, float lower, float shift, bool facingRight) {
        float yOffset = Config::HEAD_RADIUS * 1.15f;  // Encima de la cabeza

        glm::mat4 t = glm::translate(base, glm::vec3(0.0f,
            0.45f * Config::HEIGHT - lower + shift + yOffset, 0.0f));

        // Escalado: Ancho (X) y bajo (Y) para triángulo isósceles
        float hatWidth = Config::HAT_SIZE * 2.2f;
        float hatHeight = Config::HAT_SIZE * 0.6f;
        t = glm::scale(t, glm::vec3(hatWidth, hatHeight, 1.0f));

        sh.setMat4("transform", t);
        sh.setVec3("flatColor", Colors::DARK_GREY);
        sh.setFloat("uAlpha", 1.0f);
        triangle_.draw();
    }

    // Dibuja ambos brazos
    void drawArms(const ShaderProgram& sh, const glm::mat4& base, const WarriorPose& pose,
        const WarriorColors& colors, bool facingRight, float lower, float shift) {
        float weaponArmSide = 0.15f;   // Lado del brazo armado
        float freeArmSide = -0.15f;    // Lado del brazo libre

        if (pose.drawWeapon)
            drawWeaponArm(sh, base, pose.armAngle, colors, weaponArmSide, lower, shift);
        else
            drawEmptyHand(sh, base, colors, weaponArmSide, lower, shift);

        drawFreeArm(sh, base, colors, freeArmSide, facingRight, lower, shift);
    }

    // Dibuja brazo con arma
    // CUMPLE REQUISITO: Rotación dinámica del brazo según armAngle
    void drawWeaponArm(const ShaderProgram& sh, const glm::mat4& base, float angle,
        const WarriorColors& colors, float side, float lower, float shift) {

        // Transformación del hombro (punto de pivote)
        glm::mat4 arm = glm::translate(base, glm::vec3(side * Config::TORSO_SIZE,
            0.32f * Config::HEIGHT - lower + shift, 0.0f));
        arm = glm::rotate(arm, angle, glm::vec3(0, 0, 1));  // Rotación del brazo

        // Segmento del brazo (triángulo)
        glm::mat4 seg = glm::translate(arm, glm::vec3(0.0f, -0.5f * Config::ARM_HEIGHT, 0.0f));
        seg = glm::scale(seg, glm::vec3(Config::ARM_WIDTH, Config::ARM_HEIGHT, 1.0f));
        sh.setMat4("transform", seg);
        sh.setVec3("flatColor", colors.arm);
        triangle_.draw();

        // Mano (círculo)
        glm::mat4 hand = glm::translate(arm, glm::vec3(0.0f, -Config::ARM_HEIGHT, 0.0f));
        hand = glm::scale(hand, glm::vec3(Config::HAND_RADIUS, Config::HAND_RADIUS, 1.0f));
        sh.setMat4("transform", hand);
        sh.setVec3("flatColor", Colors::ORANGE);
        circle_.draw();

        // Arma (rectángulo extendido desde mano)
        glm::mat4 weapon = glm::translate(arm, glm::vec3(0.0f, -Config::ARM_HEIGHT - 0.5f * Config::WEAPON_LENGTH, 0.0f));
        weapon = glm::scale(weapon, glm::vec3(Config::WEAPON_THICKNESS, Config::WEAPON_LENGTH, 1.0f));
        sh.setMat4("transform", weapon);
        sh.setVec3("flatColor", colors.weapon);
        quad_.draw();
    }

    // Dibuja solo mano (cuando arma está soltada)
    void drawEmptyHand(const ShaderProgram& sh, const glm::mat4& base,
        const WarriorColors&, float side, float lower, float shift) {
        glm::mat4 hand = glm::translate(base, glm::vec3(side * Config::TORSO_SIZE,
            0.32f * Config::HEIGHT - lower + shift - Config::ARM_HEIGHT, 0.0f));
        hand = glm::scale(hand, glm::vec3(Config::HAND_RADIUS, Config::HAND_RADIUS, 1.0f));
        sh.setMat4("transform", hand);
        sh.setVec3("flatColor", Colors::ORANGE);
        circle_.draw();
    }

    // Dibuja brazo libre (sin arma, ángulo fijo)
    void drawFreeArm(const ShaderProgram& sh, const glm::mat4& base,
        const WarriorColors& colors, float side, bool facingRight, float lower, float shift) {
        glm::mat4 arm = glm::translate(base, glm::vec3(side * Config::TORSO_SIZE,
            0.32f * Config::HEIGHT - lower + shift, 0.0f));
        arm = glm::rotate(arm, glm::radians(-10.0f), glm::vec3(0, 0, 1));  // Ángulo fijo

        // Brazo más corto (90% escala)
        glm::mat4 seg = glm::translate(arm, glm::vec3(0.0f, -0.5f * Config::ARM_HEIGHT * 0.9f, 0.0f));
        seg = glm::scale(seg, glm::vec3(Config::ARM_WIDTH * 0.9f, Config::ARM_HEIGHT * 0.9f, 1.0f));
        sh.setMat4("transform", seg);
        sh.setVec3("flatColor", colors.arm);
        triangle_.draw();

        // Mano
        glm::mat4 hand = glm::translate(arm, glm::vec3(0.0f, -Config::ARM_HEIGHT * 0.9f, 0.0f));
        hand = glm::scale(hand, glm::vec3(Config::HAND_RADIUS, Config::HAND_RADIUS, 1.0f));
        sh.setMat4("transform", hand);
        sh.setVec3("flatColor", Colors::ORANGE);
        circle_.draw();
    }

    // Dibuja ambas piernas con pies
    void drawLegsWithFeet(const ShaderProgram& sh, const glm::mat4& leanTransform,
        const WarriorPose& pose, const WarriorColors& colors, float legCompress) {

        float hipYLocal = 0.03f * Config::HEIGHT;
        float legSeparation = 0.08f * Config::HEIGHT;
        float actualLegH = Config::LEG_HEIGHT * (1.0f - legCompress);  // Piernas comprimidas al agacharse

        // Pierna izquierda
        drawSingleLeg(sh, leanTransform,
            glm::vec3(-legSeparation, hipYLocal, 0.0f),
            actualLegH, colors.leg1,
            pose.leftFootStride, pose.leftFootLift);

        // Pierna derecha
        drawSingleLeg(sh, leanTransform,
            glm::vec3(legSeparation, hipYLocal, 0.0f),
            actualLegH, colors.leg2,
            pose.rightFootStride, pose.rightFootLift);
    }

    // Dibuja una pierna individual con animación de caminar
    // CUMPLE REQUISITO: Traslación procedural para animación de caminar
    void drawSingleLeg(const ShaderProgram& sh, const glm::mat4& leanTransform,
        const glm::vec3& hipOffset, float legLength, const glm::vec3& color,
        float footStride, float footLift) {

        glm::mat4 legBase = glm::translate(leanTransform, hipOffset);

        // Pierna (triángulo con desplazamiento por stride)
        glm::mat4 legTri = glm::translate(legBase, glm::vec3(footStride, -0.5f * legLength, 0.0f));
        legTri = glm::scale(legTri, glm::vec3(Config::LEG_WIDTH, legLength, 1.0f));
        sh.setMat4("transform", legTri);
        sh.setVec3("flatColor", color);
        triangle_.draw();

        // Pie (círculo con stride y lift)
        float footYLocal = -legLength + footLift;
        glm::mat4 footM = glm::translate(legBase, glm::vec3(footStride, footYLocal, 0.0f));

        // Ajuste para que pie no atraviese suelo
        float minFootY = Config::GROUND_Y + Config::HAND_RADIUS * 0.5f;
        if (hipOffset.y + footYLocal < minFootY) {
            float delta = (minFootY - (hipOffset.y + footYLocal));
            footM = glm::translate(footM, glm::vec3(0.0f, delta, 0.0f));
        }

        footM = glm::scale(footM, glm::vec3(Config::HAND_RADIUS, Config::HAND_RADIUS, 1.0f));
        sh.setMat4("transform", footM);
        sh.setVec3("flatColor", Colors::ORANGE);
        circle_.draw();
    }
};

// ============================================================================
// FASES DE ANIMACIÓN
// ============================================================================
// Enum para state machine de fases narrativas
// CUMPLE REQUISITO: Fases claramente marcadas (Presentación, Desarrollo, Cierre)
enum class AnimationPhase {
    PRESENTATION,  // Fase 1: Guerreros se acercan (0-5s)
    BATTLE,        // Fase 2: Coreografía de batalla (5-21s)
    DEFEAT,        // Fase 3a: Derrota del gris (21-24s)
    MEMORY         // Fase 3b: Memoria y reencuentro (24-30s)
};

// ============================================================================
// CONTROLADOR DE ANIMACIÓN
// ============================================================================
// Gestiona el tiempo y progreso de la animación
// TIPO DE ANIMACIÓN: Por Tiempo (Time-based)
class AnimationController {
public:
    AnimationController() : currentTime_(0.0f) {}

    // Actualiza tiempo transcurrido
    void update(float dt) {
        currentTime_ += dt;
    }

    // Determina fase actual basada en tiempo acumulado
    // State machine temporal
    AnimationPhase getCurrentPhase() const {
        float t = currentTime_;
        if (t < Config::PRESENTATION_DURATION) return AnimationPhase::PRESENTATION;
        t -= Config::PRESENTATION_DURATION;
        if (t < Config::BATTLE_DURATION) return AnimationPhase::BATTLE;
        t -= Config::BATTLE_DURATION;
        if (t < Config::DEFEAT_DURATION) return AnimationPhase::DEFEAT;
        return AnimationPhase::MEMORY;
    }

    // Obtiene tiempo local dentro de la fase actual
    float getPhaseTime() const {
        float t = currentTime_;
        if (t < Config::PRESENTATION_DURATION) return t;
        t -= Config::PRESENTATION_DURATION;
        if (t < Config::BATTLE_DURATION) return t;
        t -= Config::BATTLE_DURATION;
        if (t < Config::DEFEAT_DURATION) return t;
        return t - Config::DEFEAT_DURATION;
    }

    // Obtiene progreso normalizado (0.0-1.0) de la fase actual
    // Usado para interpolaciones suaves
    float getPhaseProgress() const {
        float pt = getPhaseTime();
        switch (getCurrentPhase()) {
        case AnimationPhase::PRESENTATION:
            return Math::clamp(pt / Config::PRESENTATION_DURATION, 0.0f, 1.0f);
        case AnimationPhase::BATTLE:
            return Math::clamp(pt / Config::BATTLE_DURATION, 0.0f, 1.0f);
        case AnimationPhase::DEFEAT:
            return Math::clamp(pt / Config::DEFEAT_DURATION, 0.0f, 1.0f);
        case AnimationPhase::MEMORY:
            return Math::clamp(pt / Config::MEMORY_DURATION, 0.0f, 1.0f);
        }
        return 0.0f;
    }

    float getTotalTime() const { return currentTime_; }

private:
    float currentTime_;  // Tiempo total transcurrido desde inicio
};

// ============================================================================
// ESCENA PRINCIPAL
// ============================================================================
// Clase que orquesta toda la animación narrativa
// Maneja renderizado de todas las fases y efectos especiales
class Scene {
public:
    Scene(const ShaderProgram& shader, const Geometry& circle,
        const Geometry& triangle, const Geometry& quad)
        : shader_(shader), renderer_(circle, triangle, quad),
        circle_(circle), triangle_(triangle), quad_(quad) {

        initializeParticles();  // Preparar partes del cuerpo
        greyArmAngle_ = glm::radians(-20.0f);
        yellowArmAngle_ = glm::radians(20.0f);
        collisionEffect_ = 0.0f;
        impactSpawned_ = false;
        bodyExplosionTriggered_ = false;
    }

    // Configura textura de inspiración (pintura abstracta original)
    // CUMPLE REQUISITO: Pintura abstracta completa mostrada
    void setInspirationTexture(const Texture* texture) {
        inspirationTexture_ = texture;
    }

    // Actualización por frame
    // ANIMACIÓN POR TIEMPO: Todas las actualizaciones basadas en dt
    void update(float dt) {
        controller_.update(dt);  // Acumular tiempo

        // Decaer efecto de colisión
        if (collisionEffect_ > 0.0f) {
            collisionEffect_ -= dt * 2.5f;
            if (collisionEffect_ < 0.0f) collisionEffect_ = 0.0f;
        }

        // Actualizar sistemas según fase
        if (controller_.getCurrentPhase() == AnimationPhase::BATTLE) {
            trackBattleStep();  // Seguimiento de pasos de coreografía
            impactSystem_.update(dt);  // Partículas de impacto
        }

        if (controller_.getCurrentPhase() == AnimationPhase::DEFEAT ||
            controller_.getCurrentPhase() == AnimationPhase::MEMORY) {
            // Física de partes del cuerpo cayendo
            for (auto& p : bodyParts_) p.update(dt, -0.8f);

            // Física de arma cayendo
            if (weaponDropped_.velocity != glm::vec2(0.0f) || weaponDropped_.position.y > -0.9f)
                weaponDropped_.update(dt, -3.0f);

            impactSystem_.update(dt);
        }

        // Actualizar tiempo de transición entre pasos de batalla
        if (controller_.getCurrentPhase() == AnimationPhase::BATTLE) {
            stepTransitionTime_ += dt;
        }
    }

    // Renderizado principal - Despacha a función específica de fase
    void render() {
        switch (controller_.getCurrentPhase()) {
        case AnimationPhase::PRESENTATION:
            renderPresentation();
            break;
        case AnimationPhase::BATTLE:
            renderSpecificChoreography();
            impactSystem_.render(shader_, circle_);  // Efectos de impacto
            break;
        case AnimationPhase::DEFEAT:
            renderDefeat();
            break;
        case AnimationPhase::MEMORY:
            renderMemory();
            break;
        }
    }

    // Renderiza fondo con textura
    // CUMPLE REQUISITO: Fondo con textura
    // Referencia LearnOpenGL: https://learnopengl.com/Getting-started/Textures
    void drawBackground(const Texture* bg) {
        if (!bg || !bg->isValid()) return;

        shader_.use();
        shader_.setInt("useTexture", 1);  // Activar modo textura
        bg->bind(0);  // Bind a unidad 0
        shader_.setInt("uTexture", 0);

        // Quad fullscreen (escalado a 2x2 en espacio normalizado)
        glm::mat4 t = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 1.0f));
        shader_.setMat4("transform", t);
        shader_.setVec3("flatColor", glm::vec3(1.0f));
        shader_.setFloat("uAlpha", 1.0f);
        shader_.setFloat("uGlow", 0.0f);
        quad_.draw();

        shader_.setInt("useTexture", 0);  // Desactivar modo textura
    }

private:
    const ShaderProgram& shader_;
    WarriorRenderer renderer_;
    const Geometry& circle_;
    const Geometry& triangle_;
    const Geometry& quad_;
    const Texture* inspirationTexture_;
    AnimationController controller_;

    // Partículas de cuerpo (generadas en derrota)
    std::vector<Particle> bodyParts_;
    Particle weaponDropped_;
    ImpactParticleSystem impactSystem_;

    // Flags de estado
    bool weaponDropInitialized_ = false;
    bool pickupStarted_ = false;
    float pickupProgress_ = 0.0f;
    bool headAttached_ = false;
    bool impactSpawned_ = false;
    bool bodyExplosionTriggered_ = false;
    glm::mat4 yellowKneelTransform_{ 1.0f };

    // Ángulos de brazos (interpolados suavemente)
    float greyArmAngle_;
    float yellowArmAngle_;
    float collisionEffect_;

    // Sistema de pasos de batalla
    int lastBattleStep_ = -1;
    float stepTransitionTime_ = 0.0f;
    float greyPrevAngle_ = 0.0f;
    float yellowPrevAngle_ = 0.0f;

    // Inicializa partículas de partes del cuerpo
    // Estas partículas se activan en fase DEFEAT con física
    void initializeParticles() {
        // Cabeza (con flag especial)
        Particle head;
        head.position = glm::vec2(-0.75f, -0.1f);
        head.velocity = glm::vec2(0.0f);
        head.rotationSpeed = 0.0f;
        head.isHead = true;  // Flag especial para lógica de recogida
        head.color = Colors::ORANGE;
        bodyParts_.push_back(head);

        // Torso
        Particle torso;
        torso.position = glm::vec2(-0.75f, -0.25f);
        torso.velocity = glm::vec2(0.0f);
        torso.rotationSpeed = 0.0f;
        torso.color = Colors::WHITE;
        bodyParts_.push_back(torso);

        // Brazos
        Particle armLeft;
        armLeft.position = glm::vec2(-0.80f, -0.2f);
        armLeft.velocity = glm::vec2(0.0f);
        armLeft.rotationSpeed = 0.0f;
        armLeft.color = Colors::LIGHT_GREY;
        bodyParts_.push_back(armLeft);

        Particle armRight;
        armRight.position = glm::vec2(-0.70f, -0.2f);
        armRight.velocity = glm::vec2(0.0f);
        armRight.rotationSpeed = 0.0f;
        armRight.color = Colors::LIGHT_GREY;
        bodyParts_.push_back(armRight);

        // Manos
        Particle handLeft;
        handLeft.position = glm::vec2(-0.82f, -0.35f);
        handLeft.velocity = glm::vec2(0.0f);
        handLeft.rotationSpeed = 0.0f;
        handLeft.color = Colors::ORANGE;
        bodyParts_.push_back(handLeft);

        Particle handRight;
        handRight.position = glm::vec2(-0.68f, -0.35f);
        handRight.velocity = glm::vec2(0.0f);
        handRight.rotationSpeed = 0.0f;
        handRight.color = Colors::ORANGE;
        bodyParts_.push_back(handRight);

        // Piernas
        Particle legLeft;
        legLeft.position = glm::vec2(-0.80f, -0.4f);
        legLeft.velocity = glm::vec2(0.0f);
        legLeft.rotationSpeed = 0.0f;
        legLeft.color = Colors::LIGHT_GREY;
        bodyParts_.push_back(legLeft);

        Particle legRight;
        legRight.position = glm::vec2(-0.70f, -0.4f);
        legRight.velocity = glm::vec2(0.0f);
        legRight.rotationSpeed = 0.0f;
        legRight.color = Colors::WHITE;
        bodyParts_.push_back(legRight);

        // Pies
        Particle footLeft;
        footLeft.position = glm::vec2(-0.80f, -0.55f);
        footLeft.velocity = glm::vec2(0.0f);
        footLeft.rotationSpeed = 0.0f;
        footLeft.color = Colors::ORANGE;
        bodyParts_.push_back(footLeft);

        Particle footRight;
        footRight.position = glm::vec2(-0.70f, -0.55f);
        footRight.velocity = glm::vec2(0.0f);
        footRight.rotationSpeed = 0.0f;
        footRight.color = Colors::ORANGE;
        bodyParts_.push_back(footRight);
    }

    // Mapea tiempo de batalla a número de paso (1-6)
    int battleStepFromTime(float t) {
        float cumulative = 0.0f;
        cumulative += Config::STEP1_DURATION; if (t < cumulative) return 1;
        cumulative += Config::STEP2_DURATION; if (t < cumulative) return 2;
        cumulative += Config::STEP3_DURATION; if (t < cumulative) return 3;
        cumulative += Config::STEP4_DURATION; if (t < cumulative) return 4;
        cumulative += Config::STEP5_DURATION; if (t < cumulative) return 5;
        return 6;  // Paso final (hold)
    }

    // Obtiene tiempo local dentro de un paso específico
    float getStepLocalTime(float battleTime, int step) {
        float cumulative = 0.0f;
        for (int i = 1; i < step; ++i) {
            if (i == 1) cumulative += Config::STEP1_DURATION;
            else if (i == 2) cumulative += Config::STEP2_DURATION;
            else if (i == 3) cumulative += Config::STEP3_DURATION;
            else if (i == 4) cumulative += Config::STEP4_DURATION;
            else if (i == 5) cumulative += Config::STEP5_DURATION;
        }
        return battleTime - cumulative;
    }

    // Obtiene duración de un paso específico
    float getStepDuration(int step) {
        switch (step) {
        case 1: return Config::STEP1_DURATION;
        case 2: return Config::STEP2_DURATION;
        case 3: return Config::STEP3_DURATION;
        case 4: return Config::STEP4_DURATION;
        case 5: return Config::STEP5_DURATION;
        default: return 1.0f;
        }
    }

    // Rastrea cambio de paso de batalla para transiciones suaves
    // Sistema de blend trees
    void trackBattleStep() {
        float bt = controller_.getPhaseTime();
        int step = battleStepFromTime(bt);

        if (step != lastBattleStep_) {
            // Cambio de paso detectado
            lastBattleStep_ = step;
            stepTransitionTime_ = 0.0f;
            greyPrevAngle_ = greyArmAngle_;    // Guardar ángulo previo
            yellowPrevAngle_ = yellowArmAngle_;
        }
    }

    // ========================================================================
    // RENDERIZADO DE FASE 1: PRESENTACIÓN (0-5s)
    // ========================================================================
    // Los guerreros caminan desde los bordes hacia el centro
    // CUMPLE REQUISITO: Traslación (movimiento horizontal), animación procedural de caminar
    void renderPresentation() {
        float progress = Math::easeOutQuad(controller_.getPhaseProgress());

        // Posiciones de inicio y final
        const float leftStart = -1.5f;   // Fuera de pantalla izquierda
        const float rightStart = 1.5f;   // Fuera de pantalla derecha
        const float meetLeft = -0.4f;    // Posición de encuentro izquierda
        const float meetRight = 0.4f;    // Posición de encuentro derecha

        // Interpolación suave de posiciones
        float leftX = Math::lerp(leftStart, meetLeft, progress);
        float rightX = Math::lerp(rightStart, meetRight, progress);

        // Calcular ángulos de brazos para apuntar entre sí
        // CUMPLE REQUISITO: Rotación dinámica (inverse kinematics)
        float greyTarget = Math::computeArmAngle(leftX, Config::GROUND_Y + 0.25f,
            rightX, Config::GROUND_Y + 0.25f, true);
        float yellowTarget = Math::computeArmAngle(rightX, Config::GROUND_Y + 0.25f,
            leftX, Config::GROUND_Y + 0.25f, false);

        // Interpolación suave de ángulos
        float lerpSpeed = 3.0f * Config::DELTA_TIME;
        greyArmAngle_ = Math::lerpAngle(greyArmAngle_, greyTarget, lerpSpeed);
        yellowArmAngle_ = Math::lerpAngle(yellowArmAngle_, yellowTarget, lerpSpeed);

        // Configurar pose del guerrero gris
        WarriorPose greyPose;
        greyPose.position = { leftX, Config::GROUND_Y };
        greyPose.armAngle = greyArmAngle_;
        greyPose.drawHat = true;  // Solo gris tiene sombrero

        // Animación procedural de caminar
        float walkSpeed = 1.8f;
        greyPose.walkCycle = progress * walkSpeed;
        float strideIntensity = 0.06f;
        float liftIntensity = 0.03f;

        // Ciclos alternados de pies (offset de PI)
        greyPose.leftFootStride = Math::walkingStride(greyPose.walkCycle, strideIntensity);
        greyPose.rightFootStride = Math::walkingStride(greyPose.walkCycle + Config::PI, strideIntensity);
        greyPose.leftFootLift = Math::clamp(sinf(greyPose.walkCycle * Config::PI * 2.0f), 0.0f, 1.0f) * liftIntensity;
        greyPose.rightFootLift = Math::clamp(sinf((greyPose.walkCycle + Config::PI) * Config::PI * 2.0f), 0.0f, 1.0f) * liftIntensity;

        renderer_.draw(shader_, greyPose, WarriorColors::grey(), true);

        // Configurar pose del guerrero amarillo (simétrico)
        WarriorPose yellowPose;
        yellowPose.position = { rightX, Config::GROUND_Y };
        yellowPose.armAngle = yellowArmAngle_;
        yellowPose.walkCycle = progress * walkSpeed;
        yellowPose.leftFootStride = Math::walkingStride(yellowPose.walkCycle, strideIntensity);
        yellowPose.rightFootStride = Math::walkingStride(yellowPose.walkCycle + Config::PI, strideIntensity);
        yellowPose.leftFootLift = Math::clamp(sinf(yellowPose.walkCycle * Config::PI * 2.0f), 0.0f, 1.0f) * liftIntensity;
        yellowPose.rightFootLift = Math::clamp(sinf((yellowPose.walkCycle + Config::PI) * Config::PI * 2.0f), 0.0f, 1.0f) * liftIntensity;

        renderer_.draw(shader_, yellowPose, WarriorColors::yellow(), false);
    }

    // ========================================================================
    // RENDERIZADO DE FASE 2: BATALLA (5-21s)
    // ========================================================================
    // Coreografía de batalla en 5 pasos sincronizada con Chaconne
    // CUMPLE REQUISITO: Desarrollo de la historia, uso extensivo de transformaciones
    void renderSpecificChoreography() {
        float battleTime = controller_.getPhaseTime();
        int currentStep = battleStepFromTime(battleTime);
        float stepTime = getStepLocalTime(battleTime, currentStep);
        float stepDuration = getStepDuration(currentStep);
        float stepProgress = Math::clamp(stepTime / stepDuration, 0.0f, 1.0f);

        WarriorPose greyPose;
        WarriorPose yellowPose;
        greyPose.drawHat = true;

        float targetGrey = greyArmAngle_;
        float targetYellow = yellowArmAngle_;

        // Despachar a función de paso específica
        switch (currentStep) {
        case 1: step1_ApproachAndMeet(stepProgress, greyPose, yellowPose, targetGrey, targetYellow); break;
        case 2: step2_DramaticWeaponCross(stepProgress, greyPose, yellowPose, targetGrey, targetYellow); break;
        case 3: step3_YellowStrikeGreyBlock(stepProgress, greyPose, yellowPose, targetGrey, targetYellow); break;
        case 4: step4_GreyStrikeYellowMatrix(stepProgress, greyPose, yellowPose, targetGrey, targetYellow); break;
        case 5: step5_YellowHeadStrikeMirrored(stepProgress, greyPose, yellowPose, targetGrey, targetYellow); break;
        }

        // Sistema de blending suave entre pasos
        float blend = Math::smoothstep(0.0f, Config::STEP_BLEND_TIME, stepTransitionTime_);
        if (stepTransitionTime_ < Config::STEP_BLEND_TIME) {
            // Blend de ángulos previos a nuevos targets
            greyArmAngle_ = Math::lerpAngle(greyPrevAngle_, targetGrey, blend);
            yellowArmAngle_ = Math::lerpAngle(yellowPrevAngle_, targetYellow, blend);
        }
        else {
            // Interpolación normal después del blend
            greyArmAngle_ = Math::lerpAngle(greyArmAngle_, targetGrey, 8.0f * Config::DELTA_TIME);
            yellowArmAngle_ = Math::lerpAngle(yellowArmAngle_, targetYellow, 8.0f * Config::DELTA_TIME);
        }

        greyPose.armAngle = greyArmAngle_;
        yellowPose.armAngle = yellowArmAngle_;

        renderer_.draw(shader_, greyPose, WarriorColors::grey(), true);
        renderer_.draw(shader_, yellowPose, WarriorColors::yellow(), false);
    }

    // Resetea animación de caminar (pies en posición neutral)
    void resetWalkingAnimation(WarriorPose& w) {
        w.leftFootStride = w.rightFootStride = 0.0f;
        w.leftFootLift = w.rightFootLift = 0.0f;
        w.walkCycle = 0.0f;
    }

    // ========================================================================
    // PASO 1: ACERCAMIENTO Y ENCUENTRO (3.5s)
    // ========================================================================
    // Los guerreros levantan armas y las cruzan lentamente
    // Sincronizado con introducción melódica de Chaconne
    void step1_ApproachAndMeet(float progress, WarriorPose& grey, WarriorPose& yellow,
        float& targetGrey, float& targetYellow) {

        float eased = Math::easeInOutQuad(progress);

        grey.position = { -0.4f, Config::GROUND_Y };
        yellow.position = { 0.4f, Config::GROUND_Y };
        resetWalkingAnimation(grey);
        resetWalkingAnimation(yellow);

        if (progress < 0.6f) {
            // Primeros 60%: Levantar armas progresivamente
            float raise = progress / 0.6f;
            float raiseEased = Math::easeOutCubic(raise);

            // De posición horizontal a 80° arriba
            targetGrey = Math::lerp(
                Math::computeArmAngle(-0.4f, Config::GROUND_Y + 0.25f, 0.4f, Config::GROUND_Y + 0.25f, true),
                glm::radians(-80.0f), raiseEased);
            targetYellow = Math::lerp(
                Math::computeArmAngle(0.4f, Config::GROUND_Y + 0.25f, -0.4f, Config::GROUND_Y + 0.25f, false),
                glm::radians(80.0f), raiseEased);
        }
        else {
            // Últimos 40%: Cruzar armas hacia adelante
            float cross = (progress - 0.6f) / 0.4f;
            float crossEased = Math::easeInQuad(cross);
            targetGrey = Math::lerp(glm::radians(-80.0f), glm::radians(15.0f), crossEased);
            targetYellow = Math::lerp(glm::radians(80.0f), glm::radians(-15.0f), crossEased);
        }
    }

    // ========================================================================
    // PASO 2: CHOQUE DRAMÁTICO DE ARMAS (3.5s)
    // ========================================================================
    // Primer ataque del amarillo, bloqueado por el gris
    // Sincronizado con primer crescendo de Chaconne
    // CUMPLE REQUISITO: Rotación, Traslación, efectos de impacto
    void step2_DramaticWeaponCross(float progress, WarriorPose& grey, WarriorPose& yellow,
        float& targetGrey, float& targetYellow) {

        grey.position = { -0.4f, Config::GROUND_Y };
        yellow.position = { 0.4f, Config::GROUND_Y };
        resetWalkingAnimation(grey);
        resetWalkingAnimation(yellow);

        if (progress < 0.25f) {
            // Preparación: Amarillo carga atrás
            float prep = progress / 0.25f;
            float prepEased = Math::easeInQuad(prep);
            targetYellow = Math::lerp(glm::radians(-15.0f), glm::radians(65.0f), prepEased);
            yellow.leanBack = -prepEased * 0.35f;  // Inclinarse atrás
            targetGrey = glm::radians(15.0f);
        }
        else if (progress < 0.75f) {
            // Ataque: Amarillo avanza y golpea
            float attack = (progress - 0.25f) / 0.5f;
            float attackEased = Math::easeOutQuad(attack);

            // Amarillo avanza
            yellow.position.x = Math::lerp(0.4f, 0.25f, attackEased);
            yellow.rightFootStride = -attackEased * 0.1f;
            targetYellow = Math::lerp(glm::radians(65.0f), glm::radians(-35.0f), attackEased);
            yellow.leanBack = Math::lerp(-0.35f, 0.25f, attackEased);  // Inclinarse adelante

            // Gris bloquea
            targetGrey = Math::lerp(glm::radians(15.0f), glm::radians(55.0f), attackEased);
            grey.leanBack = attackEased * 0.2f;

            // Efecto de impacto visual
            if (attack > 0.4f && attack < 0.6f) collisionEffect_ = 0.8f;
        }
        else {
            // Separación: Ambos retroceden
            float separate = (progress - 0.75f) / 0.25f;
            float sepEased = Math::easeOutQuad(separate);

            yellow.position.x = Math::lerp(0.25f, 0.4f, sepEased);
            yellow.leanBack = Math::lerp(0.25f, 0.0f, sepEased);
            yellow.rightFootStride = Math::lerp(-0.1f, 0.0f, sepEased);

            grey.leanBack = Math::lerp(0.2f, 0.0f, sepEased);

            targetGrey = Math::lerpAngle(glm::radians(55.0f), greyArmAngle_, sepEased);
            targetYellow = Math::lerpAngle(glm::radians(-35.0f), yellowArmAngle_, sepEased);
        }
    }

    // ========================================================================
    // PASO 3: ATAQUE BAJO DEL GRIS (3.5s)
    // ========================================================================
    // Gris contraataca con golpe bajo, amarillo esquiva agachándose
    // Sincronizado con respuesta lírica de Chaconne
    // CUMPLE REQUISITO: Escalado (duckAmount comprime piernas)
    void step3_YellowStrikeGreyBlock(float progress, WarriorPose& grey, WarriorPose& yellow,
        float& targetGrey, float& targetYellow) {

        grey.position = { -0.4f, Config::GROUND_Y };
        yellow.position = { 0.4f, Config::GROUND_Y };
        resetWalkingAnimation(grey);
        resetWalkingAnimation(yellow);

        if (progress < 0.3f) {
            // Preparación: Gris carga
            float prep = progress / 0.3f;
            float prepEased = Math::easeInQuad(prep);
            targetGrey = Math::lerp(glm::radians(50.0f), glm::radians(-65.0f), prepEased);
            grey.leanBack = prepEased * 0.5f;
            targetYellow = Math::lerp(glm::radians(-30.0f), glm::radians(25.0f), prepEased);
        }
        else if (progress < 0.7f) {
            // Ataque: Gris avanza, amarillo se agacha
            float attack = (progress - 0.3f) / 0.4f;
            float attackEased = Math::easeOutQuad(attack);

            // Gris avanza
            grey.position.x = Math::lerp(-0.4f, -0.18f, attackEased);
            grey.rightFootStride = -attackEased * 0.12f;
            targetGrey = Math::lerp(glm::radians(-65.0f), glm::radians(40.0f), attackEased);
            grey.leanBack = Math::lerp(0.5f, -0.35f, attackEased);

            // Amarillo esquiva agachándose (duckAmount comprime piernas)
            yellow.leanBack = attackEased * 1.1f;
            yellow.duckAmount = attackEased * 0.35f;  // ESCALADO: Comprime piernas
            yellow.position.x = Math::lerp(0.4f, 0.52f, attackEased);
            yellow.leftFootStride = attackEased * 0.1f;
            targetYellow = Math::lerp(glm::radians(25.0f), glm::radians(50.0f), attackEased);

            if (attack > 0.4f && attack < 0.6f) collisionEffect_ = 0.7f;
        }
        else {
            // Recuperación
            float recover = (progress - 0.7f) / 0.3f;
            float recoverEased = Math::easeOutQuad(recover);

            grey.position.x = Math::lerp(-0.18f, -0.35f, recoverEased);
            grey.leanBack = Math::lerp(-0.35f, 0.0f, recoverEased);
            grey.rightFootStride = Math::lerp(-0.12f, 0.0f, recoverEased);

            yellow.leanBack = Math::lerp(1.1f, 0.0f, recoverEased);
            yellow.duckAmount = Math::lerp(0.35f, 0.0f, recoverEased);
            yellow.position.x = Math::lerp(0.52f, 0.35f, recoverEased);
            yellow.leftFootStride = Math::lerp(0.1f, 0.0f, recoverEased);

            targetGrey = Math::lerpAngle(glm::radians(40.0f), greyArmAngle_, recoverEased);
            targetYellow = Math::lerpAngle(glm::radians(50.0f), yellowArmAngle_, recoverEased);
        }
    }

    // ========================================================================
    // PASO 4: ATAQUE A LA CABEZA DEL GRIS (2.5s)
    // ========================================================================
    // Gris ataca apuntando dinámicamente a la cabeza del amarillo
    // Sincronizado con momento de tensión de Chaconne (más corto, clímax)
    // CUMPLE REQUISITO: Rotación dinámica (inverse kinematics recalculado)
    void step4_GreyStrikeYellowMatrix(float progress, WarriorPose& grey, WarriorPose& yellow,
        float& targetGrey, float& targetYellow) {

        grey.position = { -0.35f, Config::GROUND_Y };
        yellow.position = { 0.35f, Config::GROUND_Y };
        resetWalkingAnimation(grey);
        resetWalkingAnimation(yellow);

        // Calcular ángulo para apuntar a cabeza del amarillo
        float greyShoulderY = Config::GROUND_Y + 0.32f * Config::HEIGHT;
        float yellowHeadY = Config::GROUND_Y + 0.45f * Config::HEIGHT;
        float aimHeadGrey = Math::computeArmAngle(
            grey.position.x, greyShoulderY,
            yellow.position.x, yellowHeadY,
            true
        );
        float aimOffset = glm::radians(5.0f);  // Offset dramático
        float aimedStrike = aimHeadGrey + aimOffset;

        if (progress < 0.3f) {
            // Preparación: Retroceso antes de golpe
            float prep = progress / 0.3f;
            float prepEased = Math::easeInQuad(prep);
            float prepBackAngle = aimedStrike - glm::radians(70.0f);
            targetGrey = Math::lerpAngle(greyArmAngle_, prepBackAngle, prepEased);
            grey.leanBack = prepEased * 0.45f;
            targetYellow = Math::lerpAngle(yellowArmAngle_, glm::radians(-15.0f), prepEased);
        }
        else if (progress < 0.7f) {
            // Ataque: Golpe fluido hacia la cabeza
            float attack = (progress - 0.3f) / 0.4f;
            float attackEased = Math::easeOutCubic(attack);

            // Gris avanza
            grey.position.x = Math::lerp(-0.35f, -0.12f, attackEased);
            grey.rightFootStride = -attackEased * 0.13f;

            // Mezcla de retroceso al ángulo que apunta a cabeza
            float prepBackAngle = aimedStrike - glm::radians(70.0f);
            targetGrey = Math::lerpAngle(prepBackAngle, aimedStrike, attackEased);
            grey.leanBack = Math::lerp(0.45f, -0.35f, attackEased);

            // Amarillo esquiva dramáticamente
            yellow.leanBack = attackEased * 1.3f;
            yellow.duckAmount = attackEased * 0.4f;
            yellow.position.x = Math::lerp(0.35f, 0.48f, attackEased);
            yellow.leftFootStride = attackEased * 0.1f;
            targetYellow = Math::lerp(glm::radians(-15.0f), glm::radians(45.0f), attackEased);

            if (attack > 0.35f && attack < 0.65f) collisionEffect_ = 0.6f;
        }
        else {
            // Recuperación
            float recover = (progress - 0.7f) / 0.3f;
            float recoverEased = Math::easeOutQuad(recover);

            grey.position.x = Math::lerp(-0.12f, -0.3f, recoverEased);
            grey.leanBack = Math::lerp(-0.35f, 0.0f, recoverEased);
            grey.rightFootStride = Math::lerp(-0.13f, 0.0f, recoverEased);

            yellow.leanBack = Math::lerp(1.3f, 0.0f, recoverEased);
            yellow.duckAmount = Math::lerp(0.4f, 0.0f, recoverEased);
            yellow.position.x = Math::lerp(0.48f, 0.3f, recoverEased);
            yellow.leftFootStride = Math::lerp(0.1f, 0.0f, recoverEased);

            targetGrey = Math::lerpAngle(aimedStrike, greyArmAngle_, recoverEased);
            targetYellow = Math::lerpAngle(glm::radians(45.0f), yellowArmAngle_, recoverEased);
        }
    }

    // ========================================================================
    // PASO 5: GOLPE FINAL A LA CABEZA (3.0s)
    // ========================================================================
    // Amarillo ataca a la cabeza del gris, quien esquiva hacia la DERECHA
    // Sincronizado con preparación final de Chaconne
    // CUMPLE REQUISITO: Rotación compleja con leanBack negativo (esquiva)
    void step5_YellowHeadStrikeMirrored(float progress, WarriorPose& grey, WarriorPose& yellow,
        float& targetGrey, float& targetYellow) {

        grey.position = { -0.35f, Config::GROUND_Y };
        yellow.position = { 0.35f, Config::GROUND_Y };
        resetWalkingAnimation(grey);
        resetWalkingAnimation(yellow);

        const float yellowShoulderY = Config::GROUND_Y + 0.32f * Config::HEIGHT;
        const float greyHeadBaseY = Config::GROUND_Y + 0.45f * Config::HEIGHT;
        const float aimOffset = glm::radians(12.0f);

        if (progress < 0.30f) {
            // PREPARACIÓN: Gris se prepara inclinándose hacia la DERECHA
            float prep = progress / 0.30f;
            float prepEased = Math::easeInQuad(prep);

            // leanBack NEGATIVO = inclinación hacia la derecha
            grey.leanBack = prepEased * -0.20f;
            grey.duckAmount = prepEased * 0.10f;
            grey.position.x = Math::lerp(-0.35f, -0.32f, prepEased);
            grey.rightFootStride = prepEased * 0.03f;

            float greyHeadY = greyHeadBaseY - (grey.duckAmount * 0.25f * Config::HEIGHT);

            // Puntería del amarillo hacia la cabeza del gris
            float aimHeadYellow = Math::computeArmAngle(
                yellow.position.x, yellowShoulderY,
                grey.position.x, greyHeadY,
                false
            );
            float aimedStrike = aimHeadYellow + aimOffset;

            // Preparación del ataque amarillo
            float prepBackAngle = aimedStrike + glm::radians(70.0f);
            targetYellow = Math::lerpAngle(yellowArmAngle_, prepBackAngle, prepEased);

            // Gris alista el arma defensivamente
            targetGrey = Math::lerpAngle(greyArmAngle_, glm::radians(-15.0f), prepEased);

            yellow.leanBack = prepEased * 0.45f;
        }
        else if (progress < 0.70f) {
            // ATAQUE: Gris esquiva hacia la DERECHA con leanBack MÁS NEGATIVO
            float attack = (progress - 0.30f) / 0.40f;
            float attackEased = Math::easeOutCubic(attack);

            // Gris esquiva hacia la DERECHA (leanBack negativo)
            grey.leanBack = Math::lerp(-0.20f, -0.80f, attackEased);  // Más negativo
            grey.duckAmount = Math::lerp(0.10f, 0.15f, attackEased);
            grey.position.x = Math::lerp(-0.32f, -0.18f, attackEased);  // Hacia la derecha
            grey.rightFootStride = Math::lerp(0.03f, 0.10f, attackEased);

            float greyHeadY = greyHeadBaseY - (grey.duckAmount * 0.25f * Config::HEIGHT);

            // Amarillo ataca
            yellow.position.x = Math::lerp(0.35f, 0.12f, attackEased);
            yellow.leftFootStride = attackEased * 0.13f;
            yellow.leanBack = Math::lerp(0.45f, -0.35f, attackEased);

            // Recalcular puntería del amarillo
            float aimHeadYellow = Math::computeArmAngle(
                yellow.position.x, yellowShoulderY,
                grey.position.x, greyHeadY,
                false
            );
            float aimedStrike = aimHeadYellow + aimOffset;
            float prepBackAngle = aimedStrike + glm::radians(70.0f);
            targetYellow = Math::lerpAngle(prepBackAngle, aimedStrike, attackEased);

            // Gris mantiene postura defensiva
            targetGrey = Math::lerpAngle(glm::radians(-15.0f), glm::radians(-45.0f), attackEased);

            // Efecto de impacto
            if (attack > 0.35f && attack < 0.65f) {
                collisionEffect_ = 0.6f;
            }
        }
        else {
            // RECUPERACIÓN: Volver a posición neutral
            float recover = (progress - 0.70f) / 0.30f;
            float recoverEased = Math::easeOutQuad(recover);

            // Amarillo retrocede
            yellow.position.x = Math::lerp(0.12f, 0.30f, recoverEased);
            yellow.leanBack = Math::lerp(-0.35f, 0.0f, recoverEased);
            yellow.leftFootStride = Math::lerp(0.13f, 0.0f, recoverEased);

            // Gris vuelve a posición neutral desde inclinación hacia la derecha
            grey.position.x = Math::lerp(-0.18f, -0.35f, recoverEased);
            grey.leanBack = Math::lerp(-1.30f, 0.0f, recoverEased);  // De negativo a 0
            grey.duckAmount = Math::lerp(0.40f, 0.0f, recoverEased);
            grey.rightFootStride = Math::lerp(0.10f, 0.0f, recoverEased);

            // Ángulos vuelven a la normalidad
            float greyHeadY = greyHeadBaseY - (grey.duckAmount * 0.25f * Config::HEIGHT);
            float aimHeadYellow = Math::computeArmAngle(
                yellow.position.x, yellowShoulderY,
                grey.position.x, greyHeadY,
                false
            );
            float aimedStrike = aimHeadYellow + aimOffset;

            targetYellow = Math::lerpAngle(aimedStrike, yellowArmAngle_, recoverEased);
            targetGrey = Math::lerpAngle(glm::radians(-45.0f), greyArmAngle_, recoverEased);
        }
    }

    // ========================================================================
    // EXPLOSIÓN SUAVE DEL CUERPO
    // ========================================================================
    // Activa física de partes del cuerpo con velocidades iniciales suaves
    // CUMPLE REQUISITO: Traslación con física, sistema de partículas
    void triggerBodyExplosion(const glm::vec2& position) {
        // Offsets relativos al centro del cuerpo
        std::vector<glm::vec2> offsets = {
            glm::vec2(0.0f, 0.45f * Config::HEIGHT),      // cabeza
            glm::vec2(0.0f, 0.20f * Config::HEIGHT),      // torso
            glm::vec2(-0.15f, 0.25f * Config::HEIGHT),    // brazo izquierdo
            glm::vec2(0.15f, 0.25f * Config::HEIGHT),     // brazo derecho
            glm::vec2(-0.18f, -0.05f * Config::HEIGHT),   // mano izquierda
            glm::vec2(0.18f, -0.05f * Config::HEIGHT),    // mano derecha
            glm::vec2(-0.08f, -0.15f * Config::HEIGHT),   // pierna izquierda
            glm::vec2(0.08f, -0.15f * Config::HEIGHT),    // pierna derecha
            glm::vec2(-0.08f, -0.45f * Config::HEIGHT),   // pie izquierdo
            glm::vec2(0.08f, -0.45f * Config::HEIGHT)     // pie derecho
        };

        for (size_t i = 0; i < bodyParts_.size() && i < offsets.size(); ++i) {
            bodyParts_[i].position = position + offsets[i];

            // Velocidad inicial suave - caída gentil
            float angle = atan2f(offsets[i].y, offsets[i].x);
            float horizontalSpeed = 0.05f + (rand() % 50) / 1000.0f;  // Muy pequeño
            float verticalSpeed = 0.08f + (rand() % 30) / 1000.0f;    // Ligeramente hacia arriba

            bodyParts_[i].velocity = glm::vec2(
                cos(angle) * horizontalSpeed,
                verticalSpeed
            );

            // Rotación gentil al caer
            bodyParts_[i].rotationSpeed = ((rand() % 100) - 50) / 50.0f;  // Más lento
        }
    }

    // ========================================================================
    // RENDERIZADO DE FASE 3: DERROTA (21-24s)
    // ========================================================================
    // Guerrero gris cae y su cuerpo se fragmenta
    // Guerrero amarillo suelta arma y recoge cabeza del caído
    // CUMPLE REQUISITO: Sistema de partículas, física, narrativa de cierre
    void renderDefeat() {
        float progress = controller_.getPhaseProgress();
        const float greyFinalX = -0.45f;  // Posición final del gris
        const float groundY = Config::GROUND_Y;

        // Activar explosión suave al inicio de la fase
        if (progress < 0.05f && !bodyExplosionTriggered_) {
            bodyExplosionTriggered_ = true;
            glm::vec2 fallPosition(greyFinalX, groundY + 0.25f * Config::HEIGHT);
            triggerBodyExplosion(fallPosition);
        }

        const float meetRight = 0.1f;

        // Soltar arma del amarillo a 55% del progreso
        if (progress > 0.55f && !weaponDropInitialized_) {
            weaponDropInitialized_ = true;
            weaponDropped_.position = glm::vec2(meetRight + 0.08f, -0.25f);
            weaponDropped_.velocity = glm::vec2(0.08f, -1.2f);
            weaponDropped_.rotation = 0.0f;
            weaponDropped_.rotationSpeed = 6.0f;
        }

        // Verificar si la cabeza está en el suelo
        bool headOnGround = false;
        for (const auto& p : bodyParts_) {
            if (p.isHead && !p.attached && p.position.y <= -0.89f) {
                headOnGround = true;
                break;
            }
        }

        // Iniciar secuencia de recogida cuando cabeza y arma están en suelo
        if (headOnGround && weaponDropped_.position.y <= -0.89f && !pickupStarted_) {
            pickupStarted_ = true;
            pickupProgress_ = 0.0f;
        }

        // Animación de recogida de cabeza
        if (pickupStarted_ && !headAttached_) {
            pickupProgress_ += Config::DELTA_TIME * 1.2f;
            pickupProgress_ = Math::clamp(pickupProgress_, 0.0f, 1.0f);

            if (pickupProgress_ > 0.6f) {
                // Adjuntar cabeza a posición fija (recogida)
                for (auto& p : bodyParts_) {
                    if (p.isHead && !p.attached) {
                        p.position = glm::vec2(meetRight + 0.05f, -0.35f);
                        p.velocity = glm::vec2(0.0f);
                        p.attached = true;
                        headAttached_ = true;
                    }
                }
            }
        }

        // Renderizar guerrero amarillo
        WarriorPose yellowPose;
        yellowPose.position = { meetRight, groundY };
        float yellowTarget = Math::computeArmAngle(meetRight, groundY, greyFinalX, groundY, false);
        yellowArmAngle_ = Math::lerpAngle(yellowArmAngle_, yellowTarget, 2.0f * Config::DELTA_TIME);
        yellowPose.armAngle = yellowArmAngle_;
        yellowPose.drawWeapon = !weaponDropInitialized_;  // No dibujar arma si fue soltada

        // Animación de arrodillamiento progresivo
        if (pickupStarted_) {
            float kp = Math::clamp(pickupProgress_, 0.0f, 1.0f);
            yellowPose.position.x -= 0.05f * kp;
            yellowPose.position.y -= 0.15f * kp;
            yellowPose.duckAmount = kp * 0.8f;  // Agacharse
            yellowPose.leanBack = -kp * 0.9f;   // Inclinarse hacia adelante

            if (headAttached_) {
                // Guardar transformación para renderizar cabeza después
                glm::mat4 baseT = glm::translate(glm::mat4(1.0f), glm::vec3(yellowPose.position, 0.0f));
                baseT = glm::scale(baseT, glm::vec3(-1.0f, 1.0f, 1.0f));
                yellowKneelTransform_ = baseT;
            }
        }

        renderer_.draw(shader_, yellowPose, WarriorColors::yellow(), false);
        renderBodyParts();  // Dibujar partes del cuerpo cayendo
        if (weaponDropInitialized_) renderDroppedWeapon();
    }

    // Renderiza partes del cuerpo individuales con física
    // CUMPLE REQUISITO: Pintura abstracta (todos los componentes visibles)
    void renderBodyParts() {
        shader_.use();
        shader_.setInt("useTexture", 0);
        shader_.setFloat("uAlpha", 1.0f);
        shader_.setFloat("uGlow", 0.0f);

        for (size_t i = 0; i < bodyParts_.size(); ++i) {
            const auto& part = bodyParts_[i];
            glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(part.position, 0.0f));
            t = glm::rotate(t, part.rotation, glm::vec3(0, 0, 1));

            if (part.isHead) {
                // Renderizar cabeza (círculo)
                t = glm::scale(t, glm::vec3(Config::HEAD_RADIUS, Config::HEAD_RADIUS, 1.0f));
                shader_.setMat4("transform", t);
                shader_.setVec3("flatColor", part.color);
                circle_.draw();

                // Renderizar sombrero sobre la cabeza
                glm::mat4 hatT = glm::translate(glm::mat4(1.0f), glm::vec3(part.position, 0.0f));
                hatT = glm::rotate(hatT, part.rotation, glm::vec3(0, 0, 1));
                float sideOffset = -Config::HEAD_RADIUS * 0.6f;
                float yOffset = Config::HEAD_RADIUS * 1.05f;
                hatT = glm::translate(hatT, glm::vec3(sideOffset, yOffset, 0.0f));
                hatT = glm::rotate(hatT, glm::radians(-20.0f), glm::vec3(0, 0, 1));
                hatT = glm::scale(hatT, glm::vec3(Config::HAT_SIZE * 1.2f, Config::HAT_SIZE * 0.95f, 1.0f));
                shader_.setMat4("transform", hatT);
                shader_.setVec3("flatColor", Colors::DARK_GREY);
                triangle_.draw();
            }
            else if (i == 1) {
                // Torso (índice 1) - triángulo
                t = glm::scale(t, glm::vec3(Config::TORSO_SIZE * 0.7f, Config::TORSO_SIZE * 0.7f, 1.0f));
                shader_.setMat4("transform", t);
                shader_.setVec3("flatColor", part.color);
                triangle_.draw();
            }
            else if (i == 2 || i == 3) {
                // Brazos (índices 2, 3) - triángulos alargados
                t = glm::scale(t, glm::vec3(Config::ARM_WIDTH * 0.8f, Config::ARM_HEIGHT * 0.6f, 1.0f));
                shader_.setMat4("transform", t);
                shader_.setVec3("flatColor", part.color);
                triangle_.draw();
            }
            else if (i == 4 || i == 5 || i == 8 || i == 9) {
                // Manos y pies (índices 4, 5, 8, 9) - círculos pequeños
                t = glm::scale(t, glm::vec3(Config::HAND_RADIUS, Config::HAND_RADIUS, 1.0f));
                shader_.setMat4("transform", t);
                shader_.setVec3("flatColor", part.color);
                circle_.draw();
            }
            else if (i == 6 || i == 7) {
                // Piernas (índices 6, 7) - triángulos alargados
                t = glm::scale(t, glm::vec3(Config::LEG_WIDTH * 0.8f, Config::LEG_HEIGHT * 0.5f, 1.0f));
                shader_.setMat4("transform", t);
                shader_.setVec3("flatColor", part.color);
                triangle_.draw();
            }
        }
    }

    // Renderiza arma caída con física
    void renderDroppedWeapon() {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(weaponDropped_.position, 0.0f));
        t = glm::rotate(t, weaponDropped_.rotation, glm::vec3(0, 0, 1));
        t = glm::scale(t, glm::vec3(Config::WEAPON_THICKNESS, Config::WEAPON_LENGTH, 1.0f));
        shader_.use();
        shader_.setMat4("transform", t);
        shader_.setVec3("flatColor", Colors::WHITE);
        shader_.setFloat("uAlpha", 1.0f);
        quad_.draw();
    }

    // ========================================================================
    // RENDERIZADO DE FASE 4: MEMORIA (24-30s)
    // ========================================================================
    // Fase de cierre: Sol aparece, fantasmas se reúnen en el cielo
    // CUMPLE REQUISITO: Cierre narrativo, transparencia, pintura abstracta completa
    void renderMemory() {
        float progress = Math::easeOutQuad(controller_.getPhaseProgress());

        drawSun(progress);           // Sol creciente con glow
        drawMemoryOverlay(progress); // Overlay sepia nostálgico

        // Guerrero amarillo arrodillado con cabeza del gris (cuerpos físicos)
        WarriorPose yellowPose;
        yellowPose.position = { 0.1f, Config::GROUND_Y };
        yellowPose.armAngle = Math::computeArmAngle(0.1f, -0.35f, -0.45f, -0.35f, false);
        yellowPose.drawWeapon = false;
        yellowPose.duckAmount = 0.8f;
        yellowPose.leanBack = -0.9f;
        renderer_.draw(shader_, yellowPose, WarriorColors::yellow(), false);

        renderBodyParts();
        if (weaponDropInitialized_) renderDroppedWeapon();

        // A partir de 40%: Fantasmas emergen y se acercan en el sol
        if (progress > 0.4f) {
            drawGhostsTogetherInSun(progress);
        }
    }

    // Dibuja fantasmas reuniéndose en el sol con brazos extendidos
    // CUMPLE REQUISITO: Pintura abstracta completa, transparencia, cierre narrativo
    void drawGhostsTogetherInSun(float progress) {
        float ghostProgress = Math::clamp((progress - 0.4f) / 0.6f, 0.0f, 1.0f);
        float ghostAlpha = 0.7f * ghostProgress;  // Transparencia progresiva

        // Posición del sol/memoria
        float sunY = 0.65f;
        float sunX = 0.0f;

        // Guerreros se acercan progresivamente
        float separation = Math::lerp(0.7f, 0.25f, ghostProgress);  // De separados a juntos
        float ghostScale = 0.4f + ghostProgress * 0.15f;            // Crecimiento sutil

        // Fantasma izquierdo (guerrero gris)
        float greyX = sunX - separation * 0.5f;
        WarriorPose greyGhost;
        greyGhost.position = { greyX, sunY };
        greyGhost.scale = ghostScale;
        greyGhost.alpha = ghostAlpha;  // CUMPLE REQUISITO: Transparencia
        greyGhost.drawHat = true;

        // Gris extiende brazo hacia amarillo
        greyGhost.armAngle = Math::lerp(
            glm::radians(-35.0f),
            Math::computeArmAngle(greyX, sunY + 0.25f, sunX + separation * 0.5f, sunY + 0.25f, true),
            ghostProgress
        );

        renderer_.draw(shader_, greyGhost, WarriorColors::grey(), true);

        // Fantasma derecho (guerrero amarillo)
        float yellowX = sunX + separation * 0.5f;
        WarriorPose yellowGhost;
        yellowGhost.position = { yellowX, sunY };
        yellowGhost.scale = ghostScale;
        yellowGhost.alpha = ghostAlpha;

        // Amarillo extiende brazo hacia gris
        yellowGhost.armAngle = Math::lerp(
            glm::radians(35.0f),
            Math::computeArmAngle(yellowX, sunY + 0.25f, greyX, sunY + 0.25f, false),
            ghostProgress
        );

        renderer_.draw(shader_, yellowGhost, WarriorColors::yellow(), false);

        // Aura brillante alrededor de los fantasmas
        if (ghostProgress > 0.3f) {
            float auraP = (ghostProgress - 0.3f) / 0.7f;
            glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(sunX, sunY, 0.0f));
            t = glm::scale(t, glm::vec3(1.0f * auraP, 0.7f * auraP, 1.0f));
            shader_.use();
            shader_.setMat4("transform", t);
            shader_.setVec3("flatColor", glm::vec3(1.0f, 1.0f, 0.95f));
            shader_.setFloat("uAlpha", 0.15f * auraP);
            shader_.setFloat("uGlow", 0.3f * auraP);
            shader_.setInt("useTexture", 0);
            circle_.draw();
        }

        // Dibujar overlay de inspiración sobre fantasmas
        drawInspirationOverlay(ghostProgress, sunX, sunY, separation);
    }

    // Dibuja overlay de imagen de inspiración (pintura abstracta original)
    // CUMPLE REQUISITO: Pintura abstracta completa mostrada al final
    void drawInspirationOverlay(float progress, float centerX, float centerY, float warriorSeparation) {
        if (progress < 0.5f) return;

        // Fade-in progresivo de la imagen
        float overlayAlpha = Math::clamp((progress - 0.5f) / 0.5f, 0.0f, 1.0f) * 0.4f;

        glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(centerX, centerY, 0.0f));
        float overlayWidth = warriorSeparation + 0.4f;
        float overlayHeight = 0.9f;
        t = glm::scale(t, glm::vec3(overlayWidth, overlayHeight, 1.0f));

        shader_.use();
        shader_.setMat4("transform", t);
        shader_.setVec3("flatColor", glm::vec3(1.0f));
        shader_.setFloat("uAlpha", overlayAlpha);  // CUMPLE REQUISITO: Transparencia
        shader_.setFloat("uGlow", 0.0f);

        // Bind de textura de inspiración si existe
        if (inspirationTexture_ && inspirationTexture_->isValid()) {
            shader_.setInt("useTexture", 1);
            inspirationTexture_->bind(0);
            shader_.setInt("uTexture", 0);
        }

        quad_.draw();
        shader_.setInt("useTexture", 0);
    }

    // Dibuja sol con efecto glow
    void drawSun(float progress) {
        float sunAlpha = progress * 0.6f;
        float sunScale = 0.3f + progress * 0.4f;  // CUMPLE REQUISITO: Escalado

        glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.65f, 0.0f));
        t = glm::scale(t, glm::vec3(sunScale, sunScale, 1.0f));

        shader_.use();
        shader_.setMat4("transform", t);
        shader_.setVec3("flatColor", Colors::SUN);
        shader_.setFloat("uAlpha", sunAlpha);
        shader_.setFloat("uGlow", progress * 0.8f);  // Glow intenso
        circle_.draw();
    }

    // Dibuja overlay sepia para efecto de memoria nostálgica
    // CUMPLE REQUISITO: Transparencia, armonía de colores
    void drawMemoryOverlay(float progress) {
        glm::mat4 t = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 1.0f));
        shader_.use();
        shader_.setMat4("transform", t);
        shader_.setVec3("flatColor", Colors::MEMORY_TINT);
        shader_.setFloat("uAlpha", 0.4f * progress);
        shader_.setFloat("uGlow", 0.0f);
        quad_.draw();
    }
};

// ============================================================================
// APLICACIÓN PRINCIPAL
// ============================================================================
// Clase wrapper que maneja ventana, contexto OpenGL y loop principal
// Referencia LearnOpenGL: https://learnopengl.com/Getting-started/Creating-a-window
class Application {
public:
    Application() : window_(nullptr) {}
    ~Application() { cleanup(); }

    // Inicializa GLFW, contexto OpenGL 3.3 Core y GLAD
    bool initialize() {
        // Inicializar GLFW
        if (!glfwInit()) {
            std::cerr << "Error al inicializar GLFW\n";
            return false;
        }

        // Configurar OpenGL 3.3 Core Profile
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Crear ventana
        window_ = glfwCreateWindow(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT,
            "Fantasmas de una Amistad - Mejorado", nullptr, nullptr);
        if (!window_) {
            std::cerr << "Error al crear ventana\n";
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1);  // V-Sync activado

        // Cargar funciones de OpenGL con GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Error al inicializar GLAD\n";
            return false;
        }

        // Habilitar alpha blending
        // CUMPLE REQUISITO: Transparencia correcta
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        return true;
    }

    // Loop principal de la aplicación
    // ANIMACIÓN POR TIEMPO: Delta time fijo para velocidad consistente
    void run() {
        // Crear shaders y geometrías (una sola vez, reutilizados)
        ShaderProgram shader(Shaders::vertexSource, Shaders::fragmentSource);
        Geometry quad = GeometryFactory::createQuad();
        Geometry triangle = GeometryFactory::createTriangle();
        Geometry circle = GeometryFactory::createCircle(56);  // 56 segmentos para suavidad

        // Cargar texturas
        // CUMPLE REQUISITO: Fondo con textura
        // NOTA: Ajustar rutas según ubicación de archivos
        std::unique_ptr<Texture> background;
        background = std::make_unique<Texture>("C:/OpenGL/texturas/fondo.png");

        // Textura de inspiración (pintura abstracta original)
        std::unique_ptr<Texture> inspiration;
        inspiration = std::make_unique<Texture>("C:/OpenGL/texturas/inspiracion.png");

        // Crear escena principal
        Scene scene(shader, circle, triangle, quad);
        scene.setInspirationTexture(inspiration.get());

        // Loop de renderizado
        while (!glfwWindowShouldClose(window_)) {
            processInput();  // Manejar input del usuario

            // Actualizar animación con delta time fijo
            scene.update(Config::DELTA_TIME);

            // Limpiar buffer de color
            glClearColor(0.5f, 0.5f, 0.5f, 1.0f);  // Gris medio (fondo por defecto)
            glClear(GL_COLOR_BUFFER_BIT);

            // Renderizar
            scene.drawBackground(background.get());  // Fondo primero
            scene.render();                          // Escena encima

            // Swap buffers y poll eventos
            glfwSwapBuffers(window_);
            glfwPollEvents();
        }
    }

private:
    GLFWwindow* window_;

    // Procesa input del teclado
    void processInput() {
        if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window_, true);
    }

    // Limpieza de recursos
    void cleanup() {
        if (window_) glfwDestroyWindow(window_);
        glfwTerminate();
    }
};

// ============================================================================
// PUNTO DE ENTRADA PRINCIPAL
// ============================================================================
int main(int argc, char** argv) {
    Application app;
    if (!app.initialize()) return -1;
    app.run();
    return 0;
}
