# Fantasmas de una Amistad
## Proyecto_1 para Gráficas por computadora
Animación 2D en OpenGL 3.3 Core
Por: Mauricia Peña y Javier Escalante)

### Descripción
Animación narrativa 2D que cuenta la historia de dos guerreros cuya amistad 
trasciende incluso la muerte. Sincronizada con la música "Chaconne" de Bach 
(Partita for Violin Solo No. 2 in D minor, BWV 1004: V).

### Historia

#### Fase 1: Presentación
Dos guerreros (gris con sombrero y amarillo) caminan uno hacia el otro desde 
los extremos de la pantalla. Sus armas apuntan inicialmente entre sí, 
estableciendo la tensión del encuentro.

#### Fase 2: Desarrollo - La Batalla 
Una coreografía de combate en 5 pasos sincronizada con la música:
1. **Approach and Meet** : Los guerreros se acercan y levantan sus armas
2. **Dramatic Weapon Cross** : Primer choque de espadas con efecto de impacto
3. **Yellow Strike, Grey Block** : El guerrero amarillo ataca, el gris se defiende
4. **Grey Strike, Yellow Matrix** : El gris contraataca apuntando a la cabeza
5. **Yellow Head Strike** : Golpe final del amarillo, el gris esquiva hacia la derecha

#### Fase 3: Derrota 
El guerrero gris cae y su cuerpo se fragmenta en partes que caen suavemente al suelo.
El guerrero amarillo suelta su arma y se arrodilla para recoger la cabeza de su amigo.

#### Fase 4: Memoria/Cierre 
Aparece un sol dorado. Los espíritus (fantasmas) de ambos guerreros emergen y se 
acercan en el cielo, sus brazos extendiéndose el uno hacia el otro. Una imagen de 
inspiración se superpone sobre ellos, representando la unión eterna de su amistad.

---

### Requisitos del Proyecto Cumplidos

#### 1. Pintura Abstracta Completa ✓
- **Ubicación:** Fase 4 (Memoria)
- **Implementación:** Los dos guerreros completos aparecen como fantasmas transparentes 
  en el sol, mostrando todos los elementos de la obra original (cabeza, torso, brazos, 
  piernas, armas, sombrero del gris)

#### 2. Transformaciones Geométricas ✓

**Traslación (Translation):**
- Guerreros caminando en Fase 1 (líneas 1485-1492)
- Movimientos de ataque/defensa en Fase 2 (líneas 1540-1650)
- Caída de partes del cuerpo en Fase 3 (líneas 1105-1113)
- Elevación de fantasmas en Fase 4 (líneas 1823-1826)

**Rotación (Rotation):**
- Rotación de brazos para ángulos de ataque (líneas 1015-1019, función `drawArms`)
- Rotación de arma al caer (líneas 1706-1710)
- Rotación de partes del cuerpo al explotar (líneas 1730-1738)
- Inclinación corporal con `leanBack` (líneas 916-920, función `drawLegsWithFeet`)

**Escalado (Scaling):**
- Escala de guerreros fantasma (línea 1814: `ghostScale = 0.4f + ghostProgress * 0.15f`)
- Escala del sol creciente (línea 1874: `sunScale = 0.3f + progress * 0.4f`)
- Escala de partículas de impacto (línea 608)
- Escala de componentes del guerrero (brazos, piernas, torso - líneas 987-1067)

#### 3. Duración y Fases ✓
- **Duración Total:** 30 segundos (excede mínimo de 15s)
- **Fases Claramente Marcadas:**
  - Presentación: 5s (líneas 172-175)
  - Desarrollo (Batalla): 16s (líneas 176-177)
  - Cierre (Derrota + Memoria): 9s (líneas 178-180)

#### 4. Fondo y Transparencia ✓

**Fondo con Textura:**
- Implementado en líneas 1345-1363 (`drawBackground`)
- Archivo: `"C:/OpenGL/texturas/fondo.png"`
- Renderizado con sampling de textura en shader (líneas 147-152)

**Objetos Transparentes:**
- **Fantasmas en Fase 4:** Alpha 0.7 (línea 1810)
- **Sol:** Alpha progresivo 0.0-0.6 (línea 1873)
- **Overlay de memoria:** Alpha 0.4 (línea 1888)
- **Imagen de inspiración:** Alpha 0.4 (línea 1851)
- **Partículas de impacto:** Alpha variable (líneas 602-615)

#### 5. Curvas (Círculos) ✓
- **Cabezas:** Círculos de 56 segmentos (línea 361, `createCircle`)
- **Manos y Pies:** Círculos pequeños (líneas 1049-1057, 1060-1067)
- **Sol:** Círculo grande con efecto glow (líneas 1872-1879)
- **Partículas:** Círculos animados (líneas 602-615)

#### 6. Música/Sonidos ✓
**Nota:** El código actual está diseñado para sincronizarse con:
- **Pieza Musical:** Bach - Chaconne, BWV 1004 V
- **Sincronización:** Duraciones de pasos de batalla calibradas a frases musicales
- **Implementación recomendada:** OpenAL, FMOD, o irrKlang (no incluido en código base)

---

### Arquitectura del Código

#### Estructura por Namespaces

**`Config`** (líneas 40-70)
- Constantes de configuración (dimensiones, tiempos)
- Proporciones anatómicas de guerreros
- Duraciones de fases y coreografía

**`Math`** (líneas 75-112)
- Funciones de interpolación: `lerp`, `lerpAngle`
- Easing functions: `easeInQuad`, `easeOutQuad`, `easeInOutQuad`, `easeOutCubic`
- Utilidades: `smoothstep`, `clamp`, `computeArmAngle`
- Funciones de animación de caminar: `walkingBob`, `walkingStride`

**`Colors`** (líneas 117-137)
- Paleta de colores melancólicos y desaturados
- Colores de piel (ORANGE), vestimenta (YELLOW, BLUE, WHITE, GREY)
- Colores ambientales (SUN, MEMORY_TINT)

**`Shaders`** (líneas 142-165)
- Vertex Shader: Transformación MVP con UVs
- Fragment Shader: Color flat/textura, alpha blending, efecto glow

#### Clases Principales

**`ShaderProgram`** (líneas 170-205)
Gestión de shaders siguiendo el patrón de LearnOpenGL:
- `compileShader()`: Compila vertex/fragment shaders
- `checkLinkErrors()`: Valida linking del programa
- `setMat4()`, `setVec3()`, `setFloat()`, `setInt()`: Uniforms
- **Referencia LearnOpenGL:** [Shaders](https://learnopengl.com/Getting-started/Shaders)

**`Geometry`** (líneas 210-230)
Contenedor de VAO/VBO/EBO siguiendo arquitectura moderna de OpenGL:
- Almacena handles de buffers OpenGL
- Método `draw()` para renderizado
- **Referencia LearnOpenGL:** [Hello Triangle](https://learnopengl.com/Getting-started/Hello-Triangle)

**`GeometryFactory`** (líneas 232-316)
Patrón Factory para crear geometría:
- `createQuad()`: Cuadrilátero para armas, fondos, texturas
- `createTriangle()`: Triángulos para torso, brazos, piernas, sombrero
- `createCircle(segments)`: Círculos paramétricos para cabezas, manos, sol
- **Referencia LearnOpenGL:** [Vertex Data](https://learnopengl.com/Getting-started/Hello-Triangle)

**`Texture`** (líneas 321-355)
Carga y gestión de texturas con stb_image:
- Soporte RGBA con canal alpha
- Mipmapping para calidad
- Wrapping y filtering modes
- **Referencia LearnOpenGL:** [Textures](https://learnopengl.com/Getting-started/Textures)

**`Particle`** (líneas 360-403)
Sistema de partículas físicas:
- Física: gravedad, velocidad, fricción, rebotes
- Rotación: `rotation`, `rotationSpeed`
- Estados: `attached` (para cabeza recogida)
- `update()`: Integración de física por frame
- **Referencia:** Conceptos de sistemas de partículas

**`ImpactParticleSystem`** (líneas 406-452)
Efectos de impacto en choques de espadas:
- `spawn()`: Genera partículas radiales en punto de colisión
- `update()`: Actualiza todas las partículas
- `render()`: Dibuja partículas con alpha decreciente
- Usado en línea 1277 durante batalla

**`WarriorPose`** (líneas 457-475)
Datos de pose/estado de un guerrero:
- Posición, escala, alpha (transparencia)
- `armAngle`: Ángulo del brazo armado
- `duckAmount`, `leanBack`: Deformaciones corporales
- `leftFootStride`, `rightFootStride`: Animación de caminar
- `drawWeapon`, `drawHat`: Flags de renderizado

**`WarriorColors`** (líneas 477-492)
Paleta de colores por guerrero:
- `grey()`: Guerrero gris (sombrero, ropa clara)
- `yellow()`: Guerrero amarillo (ropa amarilla/azul)

**`WarriorRenderer`** (líneas 497-1068)
Renderizador complejo de guerreros articulados:

- **`draw()`** (líneas 502-524): Función principal de renderizado
  - Aplica transformación base (posición, escala, flip horizontal)
  - Calcula compresión de piernas (`legCompress`) por agacharse
  - Calcula inclinación de torso (`torsoLean`) por `leanBack`
  - Dibuja componentes en orden: torso, cabeza, sombrero, brazos, piernas

- **`drawTorso()`** (líneas 532-538): Triángulo para cuerpo
  - Ajusta posición por `lower` (agacharse) y `shift` (inclinación)

- **`drawHead()`** (líneas 540-546): Círculo para cabeza
  - Offset vertical ajustado por pose

- **`drawHat()`** (líneas 548-567): Sombrero del guerrero gris
  - Triángulo isósceles ancho y bajo
  - Centrado encima de la cabeza sin rotación

- **`drawArms()`** (líneas 569-577): Dibuja ambos brazos
  - Brazo armado con `drawWeaponArm()` o `drawEmptyHand()`
  - Brazo libre con `drawFreeArm()`

- **`drawWeaponArm()`** (líneas 579-604): Brazo + mano + arma
  - Rotación por `armAngle` para apuntar
  - Arma como rectángulo extendido desde la mano

- **`drawFreeArm()`** (líneas 616-636): Brazo sin arma
  - Ángulo fijo (-10°), más corto (90% escala)

- **`drawLegsWithFeet()`** (líneas 638-657): Dibuja ambas piernas
  - Compresión por `duckAmount`
  - Llama a `drawSingleLeg()` para cada pierna

- **`drawSingleLeg()`** (líneas 659-686): Pierna individual
  - `footStride`: Desplazamiento horizontal del pie (caminar)
  - `footLift`: Elevación del pie (paso)
  - Ajuste para que pies no atraviesen el suelo (`minFootY`)

**Referencia LearnOpenGL:** [Transformations](https://learnopengl.com/Getting-started/Transformations), [Coordinate Systems](https://learnopengl.com/Getting-started/Coordinate-Systems)

**`AnimationPhase`** (línea 691)
Enum para fases de la historia: PRESENTATION, BATTLE, DEFEAT, MEMORY

**`AnimationController`** (líneas 693-727)
Controlador de tiempo y progreso de animación:
- `update(dt)`: Acumula tiempo transcurrido
- `getCurrentPhase()`: Determina fase actual basada en tiempo
- `getPhaseTime()`: Tiempo local dentro de la fase actual
- `getPhaseProgress()`: Progreso normalizado (0.0-1.0) de fase actual
- **Patrón:** State machine temporal

**`Scene`** (líneas 732-1894)
Clase principal que orquesta toda la animación:

- **Constructor** (líneas 735-747): Inicializa sistemas
  - Crea partículas de cuerpo
  - Inicializa ángulos de brazos
  - Preparar sistemas de efectos

- **`update(dt)`** (líneas 754-782): Actualización por frame
  - Actualiza controlador de tiempo
  - Decae efecto de colisión (`collisionEffect_`)
  - Actualiza partículas de impacto en batalla
  - Actualiza física de partes del cuerpo en derrota/memoria
  - Rastrea paso de coreografía en batalla (`trackBattleStep()`)

- **`render()`** (líneas 784-799): Despacha renderizado por fase
  - Switch por `getCurrentPhase()`
  - Llama a función de renderizado específica de fase

- **`trackBattleStep()`** (líneas 837-846): Sistema de pasos de batalla
  - Detecta cambio de paso
  - Inicializa transición suave entre pasos
  - Guarda ángulos previos para interpolación

- **`battleStepFromTime()`** (líneas 803-811): Mapea tiempo a paso (1-6)
  - Usa duraciones acumulativas de `Config`

- **`getStepLocalTime()`** (líneas 813-824): Tiempo local dentro de un paso

- **`getStepDuration()`** (líneas 826-835): Duración de paso específico

- **`renderPresentation()`** (líneas 1375-1425): Fase 1
  - Guerreros caminan desde bordes (-1.5, 1.5) hacia centro (-0.4, 0.4)
  - Interpolación suave con `easeOutQuad`
  - Animación de caminar: `walkingStride`, `walkingBob`
  - Brazos apuntan entre sí con `computeArmAngle`

- **`renderSpecificChoreography()`** (líneas 1427-1478): Fase 2
  - Obtiene paso actual y progreso con `battleStepFromTime()`
  - Switch entre 5 funciones de paso (step1-step5)
  - Interpolación de ángulos con blend suave en transiciones
  - Renderiza ambos guerreros con poses calculadas

- **Step Functions (líneas 1484-1688):** Coreografía detallada
  
  - **`step1_ApproachAndMeet`**: Levantan armas progresivamente
    - 60% inicial: Levantan de horizontal (-80°/80°) con `easeOutCubic`
    - 40% final: Cruzan hacia adelante (15°/-15°) con `easeInQuad`
  
  - **`step2_DramaticWeaponCross`**: Primer choque
    - 25%: Amarillo prepara (65° atrás, leanBack -0.35)
    - 50%: Amarillo ataca (-35°), gris bloquea (55°)
    - Efecto de colisión en 40-60% del ataque
    - 25%: Separación y recuperación
  
  - **`step3_YellowStrikeGreyBlock`**: Ataque bajo del gris
    - 30%: Gris carga (-65°, leanBack 0.5)
    - 40%: Gris ataca avanzando, amarillo se agacha (`duckAmount 0.35`)
    - 30%: Recuperación a posiciones defensivas
  
  - **`step4_GreyStrikeYellowMatrix`**: Gris ataca a cabeza
    - Usa `computeArmAngle` para apuntar dinámicamente a cabeza del amarillo
    - 30%: Preparación con retroceso de 70°
    - 40%: Ataque fluido con `easeOutCubic` hacia cabeza
    - Amarillo esquiva con `duckAmount` y `leanBack` extremos
  
  - **`step5_YellowHeadStrikeMirrored`**: Golpe final (simetría)
    - 30%: Preparación - gris se inclina DERECHA (`leanBack` negativo)
    - 40%: Ataque amarillo mientras gris esquiva hacia derecha
    - Puntería dinámica recalculada cada frame
    - Efecto de impacto marca inicio de derrota

- **`renderDefeat()`** (líneas 1690-1755): Fase 3
  - `triggerBodyExplosion()` al 5% de progreso (línea 1700)
  - Explosión suave de cuerpo en partes con física (líneas 1771-1806)
  - Amarillo suelta arma a 55% (línea 1709)
  - Secuencia de recogida de cabeza:
    - Detecta cabeza y arma en suelo (líneas 1718-1725)
    - Inicia pickup con `pickupProgress` (líneas 1728-1732)
    - Arrodillamiento progresivo de amarillo (`duckAmount`, `leanBack`)
    - Cabeza se adjunta (`attached = true`) a 60% de pickup

- **`renderMemory()`** (líneas 1757-1769): Fase 4
  - Dibuja sol creciente con `drawSun()` (líneas 1872-1879)
  - Overlay sepia con `drawMemoryOverlay()` (líneas 1881-1889)
  - Guerrero amarillo arrodillado con cabeza del gris
  - A partir de 40%: fantasmas emergen con `drawGhostsTogetherInSun()`

- **`drawGhostsTogetherInSun()`** (líneas 1807-1870): Fantasmas
  - Progreso 40-100% mapeado a 0-1 para fantasmas
  - Guerreros fantasma con alpha 0.7
  - Se acercan: `separation` de 0.7 a 0.25
  - Crecen ligeramente: `ghostScale` 0.4 a 0.55
  - Brazos se extienden entre sí con `computeArmAngle`
  - Aura brillante aparece a 30% de progreso fantasma
  - **Overlay de inspiración** (líneas 1845-1869):
    - Aparece a 50% con fade-in progresivo
    - Se superpone sobre fantasmas con alpha 0.4
    - Escala: ancho = separación + 0.4, alto = 0.9
    - **Textura:** `inspirationTexture_` (pintura original)

**`Application`** (líneas 1899-1961)
Clase wrapper de la aplicación OpenGL:
- `initialize()`: Setup de GLFW, contexto OpenGL 3.3 Core, GLAD
- `run()`: Loop principal de renderizado
- `processInput()`: Manejo de input (ESC para salir)
- `cleanup()`: Destrucción de recursos
- **Referencia LearnOpenGL:** [Creating a Window](https://learnopengl.com/Getting-started/Creating-a-window)

---

### Técnicas de Animación Implementadas

#### 1. Interpolación Temporal (Time-based Animation)
- **Sistema:** Todas las animaciones son basadas en tiempo real (línea 694: `currentTime_`)
- **Delta Time:** Configurado a 60 FPS constante (`Config::DELTA_TIME = 1/60`)
- **Progreso:** Cada fase calcula su progreso normalizado 0.0-1.0 (líneas 714-727)
- **Ventajas:** 
  - Velocidad consistente independiente del framerate
  - Fácil sincronización con música
  - Transiciones predecibles

#### 2. Easing Functions
Implementadas en namespace `Math` (líneas 80-89):
- **Linear:** Interpolación directa con `lerp()`
- **easeInQuad:** Aceleración (inicio lento) - usado en preparaciones de ataque
- **easeOutQuad:** Desaceleración (final lento) - usado en recuperaciones
- **easeInOutQuad:** S-curve (lento-rápido-lento) - usado en approach
- **easeOutCubic:** Desaceleración fuerte - usado en ataques dramáticos
- **smoothstep:** Interpolación suave - usado en transiciones de pasos

Estas funciones crean movimiento orgánico y natural, evitando animaciones lineales robóticas.

#### 3. Inverse Kinematics Simplificado
- **`computeArmAngle()`** (líneas 96-102): Calcula ángulo de brazo para apuntar a objetivo
- Usado para que guerreros apunten entre sí dinámicamente
- Ejemplos:
  - Línea 1394: Presentación - apuntan entre sí
  - Línea 1619: Step 4 - gris apunta a cabeza de amarillo
  - Línea 1667: Step 5 - amarillo apunta a cabeza de gris (recalcula cada frame)

#### 4. Procedural Animation
- **Caminar:** Funciones `walkingBob` y `walkingStride` (líneas 107-112)
  - Movimiento senoidal para balanceo y zancada
  - `walkCycle` controla fase (líneas 1404-1410)
  - Pies alternados con offset de PI

- **Respiración/Balance:** Potencial con `walkingBob` (no usado pero disponible)

#### 5. Blend Trees
- **Transiciones de Paso:** Sistema de blending (líneas 1469-1475)
  - Guarda ángulo previo al cambiar de paso
  - Mezcla suave durante `STEP_BLEND_TIME` (0.25s)
  - Usa `lerpAngle()` para interpolación circular correcta
  - Evita "popping" visual entre poses

#### 6. Sistema de Partículas
- **Impacto de Espadas:** (líneas 406-452)
  - Spawn radial en punto de colisión
  - 12 partículas por impacto
  - Física: gravedad, velocidad inicial, rotación
  - Alpha decay por tiempo de vida
  
- **Explosión de Cuerpo:** (líneas 1771-1806)
  - 10 partes del cuerpo (cabeza, torso, brazos, manos, piernas, pies)
  - Velocidades iniciales basadas en offsets de posición
  - Física realista: caída, rebote, fricción
  - Partes se detienen gradualmente al tocar suelo

#### 7. State Machine
- **`AnimationController`:** Máquina de estados basada en tiempo (líneas 693-727)
- **Estados:** PRESENTATION → BATTLE → DEFEAT → MEMORY
- **Transiciones:** Automáticas basadas en duraciones de `Config`
- **Sub-estados:** Battle tiene 5 pasos internos (1-5)

#### 8. Keyframe Animation
- Cada paso de batalla define keyframes implícitos:
  - Inicio (progress 0.0), medio (0.5), final (1.0)
  - Interpolación automática entre keyframes con easing
  - Ejemplo Step 2 (líneas 1527-1564):
    - Keyframe 0.0-0.25: Preparación
    - Keyframe 0.25-0.75: Ataque
    - Keyframe 0.75-1.0: Separación

---

### Sincronización Musical

#### Estructura de Chaconne
La pieza tiene estructura de variaciones sobre tema grave:
- **Introducción melódica:** Step 1 (3.5s)
- **Primer crescendo:** Step 2 (3.5s)
- **Respuesta lírica:** Step 3 (3.5s)
- **Tensión dramática:** Step 4 (2.5s) - más corto, clímax
- **Preparación final:** Step 5 (3.0s)

#### Timing Preciso
Duraciones calibradas en líneas 63-68:
```cpp
constexpr float STEP1_DURATION = 3.5f;
constexpr float STEP2_DURATION = 3.5f;
constexpr float STEP3_DURATION = 3.5f;
constexpr float STEP4_DURATION = 2.5f;
constexpr float STEP5_DURATION = 3.0f;
