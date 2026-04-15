#include "GLWidget.h"
#include "ConfigManager.h"
#include <QMatrix4x4>
#include <QRandomGenerator>
#include <QDebug>
#include <cmath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>

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
    , m_fountainCount(50)
    , m_fountainSpacing(0.35f)
    , m_startX(-8.5f)
    , m_fountainHeight(5.5f)
    , m_waterJetLength(2.2f)
    , m_waterJetTopWidth(0.08f)
    , m_waterJetBottomWidth(0.02f)
    , m_spawnRate(0.012f)
    , m_particleMinSize(0.04f)
    , m_particleMaxSize(0.07f)
    , m_particleMinLife(0.8f)
    , m_particleMaxLife(1.2f)
    , m_particleSpeedX(1.2f)
    , m_particleSpeedYMin(-3.0f)
    , m_particleSpeedYMax(-1.5f)
    , m_particleSpeedZ(1.2f)
    , m_poolWidth(18.0f)
    , m_poolDepth(12.0f)
    , m_waterAlpha(0.65f)
    , m_waterColor(0.2f, 0.65f, 0.95f)
    , m_windStrength(0.5f)
    , m_windDirection(0.3f)
    , m_spawnTimer(0.0f)
    , m_lastTime(0.0f)
    , m_timerId(0)
{
    ConfigManager::getInstance().loadConfig();
    applyConfig();

    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    m_particles.reserve(MAX_PARTICLES);
    m_waterJets.reserve(m_fountainCount);
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

void GLWidget::applyConfig()
{
    auto& config = ConfigManager::getInstance();

    m_fountainCount = config.getFountainCount();
    m_fountainSpacing = config.getFountainSpacing();
    m_fountainHeight = config.getFountainHeight();
    m_waterJetLength = config.getWaterJetLength();
    m_waterJetTopWidth = config.getWaterJetTopWidth();
    m_waterJetBottomWidth = config.getWaterJetBottomWidth();
    m_spawnRate = config.getSpawnRate();
    m_particleMinSize = config.getParticleMinSize();
    m_particleMaxSize = config.getParticleMaxSize();
    m_particleMinLife = config.getParticleMinLife();
    m_particleMaxLife = config.getParticleMaxLife();
    m_particleSpeedX = config.getParticleSpeedX();
    m_particleSpeedYMin = config.getParticleSpeedYMin();
    m_particleSpeedYMax = config.getParticleSpeedYMax();
    m_particleSpeedZ = config.getParticleSpeedZ();
    m_waterColor = config.getWaterColor();
    m_waterAlpha = config.getWaterAlpha();
    m_windStrength = config.getWindStrength();
    m_windDirection = config.getWindDirection();

    if (config.getAutoScale()) {
        // 根据水阀数量自动计算布局参数
        float totalWidth = m_fountainCount * m_fountainSpacing;
        m_startX = -totalWidth / 2.0f;
        m_poolWidth = totalWidth + 2.0f;
        m_poolDepth = 12.0f;

        // 自动调整相机距离
        float baseDistance = 15.0f;
        float distanceScale = 1.0f + (m_fountainCount - 50) / 100.0f;
        m_cameraDistance = qBound(8.0f, baseDistance * distanceScale, 25.0f);

        // 更新相机位置
        float radX = qDegreesToRadians(m_cameraAngleX);
        float radY = qDegreesToRadians(m_cameraAngleY);
        m_cameraPos.setX(m_cameraDistance * cos(radY) * sin(radX));
        m_cameraPos.setY(m_cameraDistance * sin(radY));
        m_cameraPos.setZ(m_cameraDistance * cos(radY) * cos(radX));

        qDebug() << "Auto-scaling enabled - Total width:" << totalWidth << "Pool width:" << m_poolWidth;
    }
    else {
        // 使用配置文件中的值
        m_startX = config.getStartX();
        m_poolWidth = config.getPoolWidth();
        m_poolDepth = config.getPoolDepth();
    }

    // 根据水阀数量自动计算布局参数
    // 总宽度 = 水阀个数 * 间距
    float totalWidth = m_fountainCount * m_fountainSpacing;

    // 自动计算起始X坐标，使水阀阵列居中
    m_startX = -totalWidth / 2.0f;

    // 根据水阀数量自动调整水池大小
    m_poolWidth = totalWidth + 2.0f;  // 两边各留1米边距
    m_poolDepth = 12.0f;              // 深度固定

    // 根据水阀数量自动调整相机距离
    float baseDistance = 15.0f;
    float distanceScale = 1.0f + (m_fountainCount - 50) / 100.0f;
    m_cameraDistance = qBound(8.0f, baseDistance * distanceScale, 25.0f);

    // 更新相机位置
    float radX = qDegreesToRadians(m_cameraAngleX);
    float radY = qDegreesToRadians(m_cameraAngleY);
    m_cameraPos.setX(m_cameraDistance * cos(radY) * sin(radX));
    m_cameraPos.setY(m_cameraDistance * sin(radY));
    m_cameraPos.setZ(m_cameraDistance * cos(radY) * cos(radX));

    qDebug() << "Layout calculated - Total width:" << totalWidth << "Pool width:" << m_poolWidth;
}

void GLWidget::reloadConfig()
{
    ConfigManager::getInstance().loadConfig();
    applyConfig();

    createFountains();
    m_particles.clear();

    for (int i = 0; i < qMin(200, m_fountainCount * 4); ++i) {
        int fountainId = QRandomGenerator::global()->bounded(m_fountainCount);
        createParticle(fountainId);
    }

    update();
    qDebug() << "Configuration reloaded, fountain count:" << m_fountainCount;
}

void GLWidget::createFountains()
{
    m_fountains.clear();
    m_waterJets.clear();

    // 计算总宽度
    float totalWidth = m_fountainCount * m_fountainSpacing;
    m_startX = -totalWidth / 2.0f;

    for (int i = 0; i < m_fountainCount; ++i) {
        float x = m_startX + i * m_fountainSpacing;

        m_fountains.append({
            QVector3D(x, m_fountainHeight, 0.0f),
            12.0f,
            -0.6f,
            0.0f,
            0.0f
            });

        WaterJetSegment jet;
        jet.start = QVector3D(x, m_fountainHeight - 0.3f, 0.0f);
        jet.end = jet.start + QVector3D(0.0f, -m_waterJetLength, 0.0f);
        jet.width = m_waterJetTopWidth;
        jet.life = m_waterJetBottomWidth;
        m_waterJets.append(jet);
    }

    qDebug() << "Created" << m_fountainCount << "water valves";
    qDebug() << "Start X:" << m_startX << "End X:" << (m_startX + totalWidth);
}
void GLWidget::createParticle(int fountainId)
{
    if (m_particles.size() >= MAX_PARTICLES) return;

    const auto& jet = m_waterJets[fountainId];

    Particle p;
    p.life = randomRange(m_particleMinLife, m_particleMaxLife);
    p.size = randomRange(m_particleMinSize, m_particleMaxSize);
    p.pos = jet.end + QVector3D(
        randomRange(-0.1f, 0.1f),
        -0.05f,
        randomRange(-0.1f, 0.1f)
    );

    p.vel = QVector3D(
        randomRange(-m_particleSpeedX, m_particleSpeedX),
        randomRange(m_particleSpeedYMin, m_particleSpeedYMax),
        randomRange(-m_particleSpeedZ, m_particleSpeedZ)
    );

    p.acc = QVector3D(0.0f, ConfigManager::getInstance().getGravity(), 0.0f);

    m_particles.append(p);
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    qDebug() << "OpenGL Version:" << glGetString(GL_VERSION);

    setupShaders();
    setupBuffers();

    for (int i = 0; i < 200; ++i) {
        int fountainId = QRandomGenerator::global()->bounded(m_fountainCount);
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

    // 水柱着色器
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
            float flow = fract(vTexCoord.y * 6.0 - uTime * 1.5);
            float stripe = smoothstep(0.2, 0.8, flow) * 0.2;
            vec3 color = vec3(0.3, 0.75, 1.0);
            color += vec3(0.2, 0.2, 0.15) * stripe;
            float edge = 1.0 - abs(vTexCoord.x - 0.5) * 1.2;
            float alpha = 0.65 * edge;
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

    QString poolFragmentSrc = QString(R"(
        #version 330 core
        in vec2 vTexCoord;
        out vec4 FragColor;
        void main() {
            vec3 color = vec3(%1, %2, %3);
            float alpha = %4;
            FragColor = vec4(color, alpha);
        }
    )").arg(m_waterColor.x()).arg(m_waterColor.y()).arg(m_waterColor.z()).arg(m_waterAlpha);

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
    m_poolProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, poolFragmentSrc.toStdString().c_str());
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
    glBufferData(GL_ARRAY_BUFFER, m_fountainCount * 6 * 5 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

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

    // 水池缓冲区 - 使用动态计算的大小
    float y = -1.05f;

    float poolVertices[] = {
        -m_poolWidth / 2, y, -m_poolDepth / 2, 0.0f, 0.0f,
         m_poolWidth / 2, y, -m_poolDepth / 2, 1.0f, 0.0f,
        -m_poolWidth / 2, y,  m_poolDepth / 2, 0.0f, 1.0f,

         m_poolWidth / 2, y, -m_poolDepth / 2, 1.0f, 0.0f,
         m_poolWidth / 2, y,  m_poolDepth / 2, 1.0f, 1.0f,
        -m_poolWidth / 2, y,  m_poolDepth / 2, 0.0f, 1.0f
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

        float length = m_waterJetLength + variation;

        jet.end = jet.start + QVector3D(0.0f, -length, 0.0f);
        jet.width = m_waterJetTopWidth;
        jet.life = m_waterJetBottomWidth;

        float sway = sin(timeOffset * 1.5f + i * 0.2f) * 0.02f;
        jet.end.setX(jet.end.x() + sway);
    }
}

void GLWidget::updateParticles(float dt)
{
    dt = qMin(dt, 0.033f);

    // 使用动态边界
    float boundaryX = m_poolWidth / 2.0f + 1.0f;
    float boundaryZ = m_poolDepth / 2.0f + 1.0f;

    // ... 粒子生成代码 ...

    for (int i = 0; i < m_particles.size(); ++i) {
        auto& p = m_particles[i];

        p.vel += p.acc * dt;
        p.pos += p.vel * dt;
        p.life -= dt * 0.35f;
        p.vel *= 0.998f;

        if (p.pos.y() <= GROUND_Y) {
            m_particles.removeAt(i);
            i--;
            continue;
        }

        // 使用动态边界
        if (fabs(p.pos.x()) > boundaryX ||
            fabs(p.pos.z()) > boundaryZ ||
            p.life <= 0.0f) {
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

    // 1. 水池
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_poolProgram.bind();
    m_poolProgram.setUniformValue(m_uniformMVP, mvp);
    glBindVertexArray(m_poolVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    m_poolProgram.release();

    // 2. 水柱
    if (m_waterJets.size() > 0) {
        std::vector<float> jetVertices;

        for (const auto& jet : m_waterJets) {
            QVector3D start = jet.start;
            QVector3D end = jet.end;

            float topWidth = jet.width;
            float bottomWidth = jet.life;

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
            m_jetProgram.setUniformValue(m_uniformTime, now);

            glBindVertexArray(m_jetVAO);
            glDrawArrays(GL_TRIANGLES, 0, jetVertices.size() / 5);
            glBindVertexArray(0);

            m_jetProgram.release();
        }
    }

    // 3. 粒子
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

    // 4. 水阀
    m_valveProgram.bind();
    m_valveProgram.setUniformValue(m_uniformMVP, mvp);
    glBindVertexArray(m_valveVAO);
    glDrawArrays(GL_TRIANGLES, 0, m_fountains.size() * 36);
    glBindVertexArray(0);
    m_valveProgram.release();
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
            int fid = QRandomGenerator::global()->bounded(m_fountainCount);
            createParticle(fid);
        }
        qDebug() << "Added particles";
        break;
    case Qt::Key_Up:
        m_spawnRate = qMax(0.005f, m_spawnRate - 0.002f);
        qDebug() << "Spawn rate:" << m_spawnRate;
        break;
    case Qt::Key_Down:
        m_spawnRate = qMin(0.05f, m_spawnRate + 0.002f);
        qDebug() << "Spawn rate:" << m_spawnRate;
        break;
    case Qt::Key_L:
        reloadConfig();
        qDebug() << "Reloaded configuration";
        break;
    case Qt::Key_R:
        m_particles.clear();
        qDebug() << "Cleared particles";
        break;
    }
}