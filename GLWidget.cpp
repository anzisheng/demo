#include "GLWidget.h"
#include <QMatrix4x4>
#include <QRandomGenerator>
#include <QDebug>
#include <cmath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>

const float GLWidget::GRAVITY = -9.8f;
const float GLWidget::GROUND_Y = -1.0f;

inline float randomRange(float min, float max) {
    return min + QRandomGenerator::global()->generateDouble() * (max - min);
}

GLWidget::GLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_cameraPos(0.0f, 5.0f, 15.0f)
    , m_cameraTarget(0.0f, 2.5f, 0.0f)
    , m_cameraDistance(15.0f)
    , m_cameraAngleX(0.0f)
    , m_cameraAngleY(30.0f)
    , m_mousePressed(false)
    , m_spawnTimer(0.0f)
    , m_spawnRate(0.01f)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    m_particles.reserve(MAX_PARTICLES);
    m_waterJets.reserve(FOUNTAIN_COUNT);
    createFountains();
}

GLWidget::~GLWidget()
{
    makeCurrent();
    if (m_particleVBO) glDeleteBuffers(1, &m_particleVBO);
    if (m_particleVAO) glDeleteVertexArrays(1, &m_particleVAO);
    if (m_jetVBO) glDeleteBuffers(1, &m_jetVBO);
    if (m_jetVAO) glDeleteVertexArrays(1, &m_jetVAO);
    if (m_valveVAO) glDeleteVertexArrays(1, &m_valveVAO);
    if (m_poolVBO) glDeleteBuffers(1, &m_poolVBO);
    if (m_poolVAO) glDeleteVertexArrays(1, &m_poolVAO);
    doneCurrent();
}

void GLWidget::createFountains()
{
    m_fountains.clear();
    m_waterJets.clear();

    float startX = -8.5f;
    float spacing = 0.35f;

    for (int i = 0; i < FOUNTAIN_COUNT; ++i) {
        float x = startX + i * spacing;

        m_fountains.append({
            QVector3D(x, 5.5f, 0.0f),
            12.0f,
            -0.6f,
            0.0f,
            0.0f
            });

        WaterJetSegment jet;
        jet.start = QVector3D(x, 5.2f, 0.0f);
        jet.end = jet.start;
        jet.width = 0.1f;
        jet.life = 1.0f;
        m_waterJets.append(jet);
    }

    qDebug() << "=== 50 Water Valves ===";
}

void GLWidget::createParticle(int fountainId)
{
    if (m_particles.size() >= MAX_PARTICLES) return;

    const auto& jet = m_waterJets[fountainId];

    Particle p;
    p.life = randomRange(0.8f, 1.2f);
    p.size = randomRange(0.04f, 0.07f);
    p.pos = jet.end + QVector3D(
        randomRange(-0.1f, 0.1f),
        -0.05f,
        randomRange(-0.1f, 0.1f)
    );

    p.vel = QVector3D(
        randomRange(-1.2f, 1.2f),    // 水平乱飞
        randomRange(-3.0f, -1.5f),   // 向下
        randomRange(-1.2f, 1.2f)     // 水平乱飞
    );

    p.acc = QVector3D(0.0f, GRAVITY, 0.0f);

    m_particles.append(p);
}
void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    qDebug() << "OpenGL Version:" << glGetString(GL_VERSION);

    setupShaders();
    setupBuffers();

    for (int i = 0; i < 200; ++i) {
        int fountainId = QRandomGenerator::global()->bounded(FOUNTAIN_COUNT);
        createParticle(fountainId);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.02f, 0.03f, 0.08f, 1.0f);

    m_elapsedTimer.start();
    m_lastTime = 0;
    m_timerId = startTimer(16);
}

void GLWidget::setupShaders()
{
    // 粒子着色器
    const char* particleVertexSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aTexCoord;
        out vec2 vTexCoord;
        uniform mat4 uMVP;
        void main() {
            vTexCoord = aTexCoord;
            gl_Position = uMVP * vec4(aPos, 1.0);
        }
    )";

    const char* particleFragmentSrc = R"(
        #version 330 core
        in vec2 vTexCoord;
        out vec4 FragColor;
        uniform float uTime;
        void main() {
            vec2 coord = vTexCoord;
            float dist = length(coord - vec2(0.5));
            if (dist > 0.5) discard;
            float alpha = (1.0 - dist) * 0.85;
            vec3 color = vec3(0.2, 0.7, 1.0);
            FragColor = vec4(color, alpha);
        }
    )";

    const char* jetVertexSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aTexCoord;
        out vec2 vTexCoord;
        uniform mat4 uMVP;
        void main() {
            vTexCoord = aTexCoord;
            gl_Position = uMVP * vec4(aPos, 1.0);
        }
    )";

    const char* jetFragmentSrc = R"(
    #version 330 core
    in vec2 vTexCoord;
    out vec4 FragColor;
    uniform float uTime;
    void main() {
        // 微弱的流动感
        float flow = fract(vTexCoord.y * 4.0 - uTime);
        float stripe = smoothstep(0.4, 0.6, flow) * 0.1;
        
        // 非常淡的蓝色
        vec3 color = vec3(0.4, 0.75, 1.0);
        color += vec3(0.1, 0.1, 0.05) * stripe;
        
        // 高透明度（像玻璃）
        float edge = 1.0 - abs(vTexCoord.x - 0.5) * 0.8;
        float alpha = 0.6 * edge;
        
        FragColor = vec4(color, alpha);
    }
)";
    // 水阀着色器
    const char* valveVertexSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;
        out vec3 vColor;
        uniform mat4 uMVP;
        void main() {
            vColor = aColor;
            gl_Position = uMVP * vec4(aPos, 1.0);
        }
    )";

    const char* valveFragmentSrc = R"(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(vColor, 1.0);
        }
    )";

    // 水池着色器
    const char* poolVertexSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aTexCoord;
        out vec2 vTexCoord;
        uniform mat4 uMVP;
        void main() {
            vTexCoord = aTexCoord;
            gl_Position = uMVP * vec4(aPos, 1.0);
        }
    )";

    const char* poolFragmentSrc = R"(
        #version 330 core
        in vec2 vTexCoord;
        out vec4 FragColor;
        void main() {
            vec3 color = vec3(0.2, 0.5, 0.9);
            float alpha = 0.55;
            FragColor = vec4(color, alpha);
        }
    )";

    m_particleProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, particleVertexSrc);
    m_particleProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, particleFragmentSrc);
    m_particleProgram.link();

    m_jetProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, jetVertexSrc);
    m_jetProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, jetFragmentSrc);
    m_jetProgram.link();

    m_valveProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, valveVertexSrc);
    m_valveProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, valveFragmentSrc);
    m_valveProgram.link();

    m_poolProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, poolVertexSrc);
    m_poolProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, poolFragmentSrc);
    m_poolProgram.link();

    m_uniformMVP = m_particleProgram.uniformLocation("uMVP");
    m_uniformTime = m_particleProgram.uniformLocation("uTime");

    qDebug() << "Shaders compiled and linked";
}

void GLWidget::setupBuffers()
{
    // 粒子缓冲区
    glGenVertexArrays(1, &m_particleVAO);
    glBindVertexArray(m_particleVAO);

    glGenBuffers(1, &m_particleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_particleVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 6 * 5 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    // 水柱缓冲区
    glGenVertexArrays(1, &m_jetVAO);
    glBindVertexArray(m_jetVAO);

    glGenBuffers(1, &m_jetVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_jetVBO);
    glBufferData(GL_ARRAY_BUFFER, FOUNTAIN_COUNT * 6 * 5 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    // 水阀缓冲区
    std::vector<float> valveVertices;
    std::vector<float> valveColors;

    for (const auto& fountain : m_fountains) {
        QVector3D pos = fountain.position;

        float w = 0.3f;
        float h = 0.5f;
        float d = 0.3f;

        float vertices[] = {
            -w, -h,  d,  w, -h,  d,  w,  h,  d,  -w,  h,  d,
            -w, -h, -d, -w,  h, -d,  w,  h, -d,  w, -h, -d,
            -w, -h, -d, -w,  h, -d, -w,  h,  d, -w, -h,  d,
             w, -h, -d,  w, -h,  d,  w,  h,  d,  w,  h, -d,
            -w,  h, -d, -w,  h,  d,  w,  h,  d,  w,  h, -d,
            -w, -h, -d,  w, -h, -d,  w, -h,  d, -w, -h,  d
        };

        float colors[] = {
            0.6f,0.5f,0.4f, 0.6f,0.5f,0.4f, 0.6f,0.5f,0.4f, 0.6f,0.5f,0.4f,
            0.6f,0.5f,0.4f, 0.6f,0.5f,0.4f, 0.6f,0.5f,0.4f, 0.6f,0.5f,0.4f,
            0.6f,0.5f,0.4f, 0.6f,0.5f,0.4f, 0.6f,0.5f,0.4f, 0.6f,0.5f,0.4f,
            0.6f,0.5f,0.4f, 0.6f,0.5f,0.4f, 0.6f,0.5f,0.4f, 0.6f,0.5f,0.4f,
            0.8f,0.7f,0.3f, 0.8f,0.7f,0.3f, 0.8f,0.7f,0.3f, 0.8f,0.7f,0.3f,
            0.5f,0.4f,0.3f, 0.5f,0.4f,0.3f, 0.5f,0.4f,0.3f, 0.5f,0.4f,0.3f
        };

        for (int i = 0; i < 36; i++) {
            valveVertices.push_back(pos.x() + vertices[i * 3]);
            valveVertices.push_back(pos.y() + vertices[i * 3 + 1]);
            valveVertices.push_back(pos.z() + vertices[i * 3 + 2]);
            valveColors.push_back(colors[i * 3]);
            valveColors.push_back(colors[i * 3 + 1]);
            valveColors.push_back(colors[i * 3 + 2]);
        }
    }

    glGenVertexArrays(1, &m_valveVAO);
    glBindVertexArray(m_valveVAO);

    GLuint valveVBO_pos, valveVBO_color;
    glGenBuffers(1, &valveVBO_pos);
    glBindBuffer(GL_ARRAY_BUFFER, valveVBO_pos);
    glBufferData(GL_ARRAY_BUFFER, valveVertices.size() * sizeof(float), valveVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glGenBuffers(1, &valveVBO_color);
    glBindBuffer(GL_ARRAY_BUFFER, valveVBO_color);
    glBufferData(GL_ARRAY_BUFFER, valveColors.size() * sizeof(float), valveColors.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // 水池缓冲区
    float poolWidth = 18.0f;
    float poolDepth = 12.0f;
    float y = -1.05f;

    float poolVertices[] = {
        -poolWidth / 2, y, -poolDepth / 2, 0.0f, 0.0f,
         poolWidth / 2, y, -poolDepth / 2, 1.0f, 0.0f,
        -poolWidth / 2, y,  poolDepth / 2, 0.0f, 1.0f,

         poolWidth / 2, y, -poolDepth / 2, 1.0f, 0.0f,
         poolWidth / 2, y,  poolDepth / 2, 1.0f, 1.0f,
        -poolWidth / 2, y,  poolDepth / 2, 0.0f, 1.0f
    };

    glGenVertexArrays(1, &m_poolVAO);
    glBindVertexArray(m_poolVAO);

    glGenBuffers(1, &m_poolVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_poolVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(poolVertices), poolVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    qDebug() << "All buffers setup complete";
}
void GLWidget::updateWaterJets(float dt)
{
    static float timeOffset = 0.0f;
    timeOffset += dt;

    for (int i = 0; i < m_waterJets.size(); ++i) {
        auto& jet = m_waterJets[i];
        const auto& fountain = m_fountains[i];

        jet.start = QVector3D(fountain.position.x(), fountain.position.y() - 0.3f, fountain.position.z());

        float freq = 0.3f + (i % 7) * 0.05f;
        float phase = i * 0.5f;
        float variation = sin(timeOffset * freq + phase) * 0.45f;

        float baseLength = 2.2f;
        float length = baseLength + variation;

        jet.end = jet.start + QVector3D(0.0f, -length, 0.0f);

        // 水柱更细，边缘更透明看起来更真实
        //float topWidth = 0.08f;   // 从 0.12 减到 0.08
        //float bottomWidth = 0.02f;
        float topWidth = 0.07f;
        float bottomWidth = 0.02f;
        jet.width = topWidth;
        jet.life = bottomWidth;

        // 轻微摆动
        float sway = sin(timeOffset * 1.5f + i * 0.2f) * 0.02f;
        jet.end.setX(jet.end.x() + sway);
    }
}
void GLWidget::updateParticles(float dt)
{
    dt = qMin(dt, 0.033f);

    // 生成新粒子...
    m_spawnTimer += dt;
    while (m_spawnTimer >= m_spawnRate && m_particles.size() < MAX_PARTICLES - 100) {
        m_spawnTimer -= m_spawnRate;

        int particlesToCreate = QRandomGenerator::global()->bounded(2, 4);
        for (int k = 0; k < particlesToCreate; ++k) {
            int fountainId = QRandomGenerator::global()->bounded(FOUNTAIN_COUNT);
            const auto& jet = m_waterJets[fountainId];
            if ((jet.start - jet.end).length() > 0.3f) {
                createParticle(fountainId);
                if (QRandomGenerator::global()->generateDouble() < 0.3f) {
                    Particle splash;
                    splash.life = randomRange(0.3f, 0.6f);
                    splash.size = randomRange(0.02f, 0.04f);
                    splash.pos = jet.end + QVector3D(
                        randomRange(-0.15f, 0.15f),
                        -0.02f,
                        randomRange(-0.15f, 0.15f)
                    );
                    splash.vel = QVector3D(
                        randomRange(-2.0f, 2.0f),
                        randomRange(-1.0f, 1.0f),
                        randomRange(-2.0f, 2.0f)
                    );
                    splash.acc = QVector3D(0.0f, GRAVITY, 0.0f);
                    m_particles.append(splash);
                }
            }
        }
    }

    // 更新粒子
    for (int i = 0; i < m_particles.size(); ++i) {
        auto& p = m_particles[i];

        // 添加持续乱飞效果（空气扰动）
        float turbulence = 0.5f;
        p.vel += QVector3D(
            randomRange(-turbulence, turbulence) * dt,
            randomRange(-turbulence * 0.5f, turbulence * 0.5f) * dt,
            randomRange(-turbulence, turbulence) * dt
        );

        p.vel += p.acc * dt;
        p.pos += p.vel * dt;
        p.life -= dt * 0.35f;
        p.vel *= 0.995f;  // 轻微阻力

        float waterY = -1.0f;

        if (p.pos.y() <= waterY) {
            m_particles.removeAt(i);
            i--;
            continue;
        }

        if (fabs(p.pos.x()) > 12.0f || fabs(p.pos.z()) > 8.0f || p.life <= 0.0f) {
            m_particles.removeAt(i);
            i--;
        }
    }
}

void GLWidget::paintGL()
{
    float now = m_elapsedTimer.elapsed() / 1000.0f;
    float dt = qMin(0.033f, now - m_lastTime);
    if (dt > 0.001f) {
        m_lastTime = now;
        updateWaterJets(dt);
        updateParticles(dt);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 view;
    view.lookAt(m_cameraPos, m_cameraTarget, QVector3D(0.0f, 1.0f, 0.0f));
    QMatrix4x4 mvp = m_projection * view;

    // 1. 绘制水池地面
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_poolProgram.bind();
    m_poolProgram.setUniformValue(m_uniformMVP, mvp);
    glBindVertexArray(m_poolVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    m_poolProgram.release();

    // 2. 绘制水柱
    if (m_waterJets.size() > 0) {
        std::vector<float> jetVertices;

        for (const auto& jet : m_waterJets) {
            QVector3D start = jet.start;
            QVector3D end = jet.end;

            float topWidth = 0.1f;
            float bottomWidth = 0.06f;

            jetVertices.push_back(start.x() - topWidth); jetVertices.push_back(start.y()); jetVertices.push_back(start.z()); jetVertices.push_back(0.0f); jetVertices.push_back(0.0f);
            jetVertices.push_back(start.x() + topWidth); jetVertices.push_back(start.y()); jetVertices.push_back(start.z()); jetVertices.push_back(1.0f); jetVertices.push_back(0.0f);
            jetVertices.push_back(end.x() - bottomWidth); jetVertices.push_back(end.y()); jetVertices.push_back(end.z()); jetVertices.push_back(0.0f); jetVertices.push_back(1.0f);

            jetVertices.push_back(start.x() + topWidth); jetVertices.push_back(start.y()); jetVertices.push_back(start.z()); jetVertices.push_back(1.0f); jetVertices.push_back(0.0f);
            jetVertices.push_back(end.x() + bottomWidth); jetVertices.push_back(end.y()); jetVertices.push_back(end.z()); jetVertices.push_back(1.0f); jetVertices.push_back(1.0f);
            jetVertices.push_back(end.x() - bottomWidth); jetVertices.push_back(end.y()); jetVertices.push_back(end.z()); jetVertices.push_back(0.0f); jetVertices.push_back(1.0f);
        }

        if (!jetVertices.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, m_jetVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, jetVertices.size() * sizeof(float), jetVertices.data());

            m_jetProgram.bind();
            m_jetProgram.setUniformValue(m_uniformMVP, mvp);

            glBindVertexArray(m_jetVAO);
            glDrawArrays(GL_TRIANGLES, 0, jetVertices.size() / 5);
            glBindVertexArray(0);

            m_jetProgram.release();
        }
    }

    // 3. 绘制水滴粒子
    if (m_particles.size() > 0) {
        std::vector<float> vertices;
        vertices.reserve(m_particles.size() * 6 * 5);

        for (const auto& p : m_particles) {
            float x = p.pos.x();
            float y = p.pos.y();
            float z = p.pos.z();
            float size = p.size * (0.5f + p.life * 0.8f);

            vertices.push_back(x - size); vertices.push_back(y - size); vertices.push_back(z); vertices.push_back(0.0f); vertices.push_back(0.0f);
            vertices.push_back(x + size); vertices.push_back(y - size); vertices.push_back(z); vertices.push_back(1.0f); vertices.push_back(0.0f);
            vertices.push_back(x - size); vertices.push_back(y + size); vertices.push_back(z); vertices.push_back(0.0f); vertices.push_back(1.0f);

            vertices.push_back(x + size); vertices.push_back(y - size); vertices.push_back(z); vertices.push_back(1.0f); vertices.push_back(0.0f);
            vertices.push_back(x + size); vertices.push_back(y + size); vertices.push_back(z); vertices.push_back(1.0f); vertices.push_back(1.0f);
            vertices.push_back(x - size); vertices.push_back(y + size); vertices.push_back(z); vertices.push_back(0.0f); vertices.push_back(1.0f);
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_particleVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

        m_particleProgram.bind();
        m_particleProgram.setUniformValue(m_uniformMVP, mvp);
        m_particleProgram.setUniformValue(m_uniformTime, now);

        glBindVertexArray(m_particleVAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 5);
        glBindVertexArray(0);

        m_particleProgram.release();
    }

    // 4. 绘制水阀
    m_valveProgram.bind();
    m_valveProgram.setUniformValue(m_uniformMVP, mvp);
    glBindVertexArray(m_valveVAO);
    glDrawArrays(GL_TRIANGLES, 0, m_fountains.size() * 36);
    glBindVertexArray(0);
    m_valveProgram.release();

    static int frameCount = 0;
    if (++frameCount % 60 == 0) {
        // qDebug() << "Particles:" << m_particles.size();
    }
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);

    float aspect = float(w) / float(h);
    m_projection.setToIdentity();
    m_projection.perspective(45.0f, aspect, 0.1f, 100.0f);
}

void GLWidget::timerEvent(QTimerEvent*)
{
    update();
}

void GLWidget::wheelEvent(QWheelEvent* event)
{
    float delta = event->angleDelta().y() / 120.0f;
    m_cameraDistance -= delta * 0.5f;
    m_cameraDistance = qBound(5.0f, m_cameraDistance, 25.0f);

    float radX = qDegreesToRadians(m_cameraAngleX);
    float radY = qDegreesToRadians(m_cameraAngleY);
    m_cameraPos.setX(m_cameraDistance * cos(radY) * sin(radX));
    m_cameraPos.setY(m_cameraDistance * sin(radY));
    m_cameraPos.setZ(m_cameraDistance * cos(radY) * cos(radX));

    update();
}

void GLWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = true;
        m_lastMousePos = event->pos();
    }
}

void GLWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_mousePressed) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_cameraAngleX += delta.x() * 0.5f;
        m_cameraAngleY += delta.y() * 0.5f;
        m_cameraAngleY = qBound(-89.0f, m_cameraAngleY, 89.0f);

        float radX = qDegreesToRadians(m_cameraAngleX);
        float radY = qDegreesToRadians(m_cameraAngleY);
        m_cameraPos.setX(m_cameraDistance * cos(radY) * sin(radX));
        m_cameraPos.setY(m_cameraDistance * sin(radY));
        m_cameraPos.setZ(m_cameraDistance * cos(radY) * cos(radX));

        m_lastMousePos = event->pos();
        update();
    }
}

void GLWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = false;
    }
}

void GLWidget::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Space:
        for (int i = 0; i < 50; ++i) {
            int fid = QRandomGenerator::global()->bounded(FOUNTAIN_COUNT);
            createParticle(fid);
        }
        qDebug() << "Added particles";
        break;
    case Qt::Key_Up:
        m_spawnRate = qMax(0.01f, m_spawnRate - 0.005f);
        qDebug() << "Spawn rate:" << m_spawnRate;
        break;
    case Qt::Key_Down:
        m_spawnRate = qMin(0.1f, m_spawnRate + 0.005f);
        qDebug() << "Spawn rate:" << m_spawnRate;
        break;
    case Qt::Key_R:
        m_particles.clear();
        qDebug() << "Cleared particles";
        break;
    }
}