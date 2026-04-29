#include "GLWidget.h"
#include "ConfigManager.h"
#include "MusicPlayer.h"
#include <QMatrix4x4>
#include <QRandomGenerator>
#include <QDebug>
#include <cmath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QDir>
#include <algorithm>

const float GLWidget::GROUND_Y = -1.0f;

inline float randomRange(float min, float max) {
    return min + QRandomGenerator::global()->generateDouble() * (max - min);
}

GLWidget::GLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_cameraPos(0.0f, 5.0f, 15.0f)
    , m_cameraTarget(0.0f, 2.0f, 0.0f)
    , m_cameraDistance(15.0f)
    , m_cameraAngleX(0.0f)
    , m_cameraAngleY(35.0f)
    , m_mousePressed(false)
    , m_spawnRate(0.012f)
    , m_particleMinSize(0.04f)
    , m_particleMaxSize(0.07f)
    , m_particleMinLife(0.8f)
    , m_particleMaxLife(1.2f)
    , m_particleSpeedX(1.2f)
    , m_particleSpeedYMin(-3.0f)
    , m_particleSpeedYMax(-1.5f)
    , m_particleSpeedZ(1.2f)
    , m_gravity(-9.8f)
    , m_poolWidth(18.0f)
    , m_poolDepth(12.0f)
    , m_waterColor(0.2f, 0.65f, 0.95f)
    , m_waterAlpha(0.65f)
    , m_windStrength(0.5f)
    , m_windDirection(0.3f)
    , m_musicSyncEnabled(false)
    , m_musicSensitivity(1.0f)
    , m_spawnTimer(0.0f)
    , m_lastTime(0.0f)
    , m_timerId(0)
    , m_initialized(false)
{
    ConfigManager::getInstance().loadConfig();
    applyConfig();
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    m_particles.reserve(MAX_PARTICLES);
}

GLWidget::~GLWidget()
{
    makeCurrent();
    if (m_particleVBO) glDeleteBuffers(1, &m_particleVBO);
    if (m_particleVAO) glDeleteVertexArrays(1, &m_particleVAO);
    if (m_jetVBO) glDeleteBuffers(1, &m_jetVBO);
    if (m_jetVAO) glDeleteVertexArrays(1, &m_jetVAO);
    if (m_poolVBO) glDeleteBuffers(1, &m_poolVBO);
    if (m_poolVAO) glDeleteVertexArrays(1, &m_poolVAO);
    if (m_curtainVBO) glDeleteBuffers(1, &m_curtainVBO);
    if (m_curtainVAO) glDeleteVertexArrays(1, &m_curtainVAO);
    if (m_curtain.textureId) glDeleteTextures(1, &m_curtain.textureId);
    doneCurrent();
}

void GLWidget::applyConfig()
{
    auto& config = ConfigManager::getInstance();
    m_valveGridWidth = config.getWaterValveGridWidth();
    m_valveGridHeight = config.getWaterValveGridHeight();
    m_valveSpacing = config.getWaterValveSpacing();
    m_valveBaseHeight = config.getWaterValveBaseHeight();
    m_valveMaxLength = config.getWaterValveMaxLength();

    m_spawnRate = config.getSpawnRate();
    m_particleMinSize = config.getParticleMinSize();
    m_particleMaxSize = config.getParticleMaxSize();
    m_particleMinLife = config.getParticleMinLife();
    m_particleMaxLife = config.getParticleMaxLife();
    m_particleSpeedX = config.getParticleSpeedX();
    m_particleSpeedYMin = config.getParticleSpeedYMin();
    m_particleSpeedYMax = config.getParticleSpeedYMax();
    m_particleSpeedZ = config.getParticleSpeedZ();
    m_gravity = config.getGravity();
    m_poolWidth = config.getPoolWidth();
    m_poolDepth = config.getPoolDepth();
    m_waterColor = config.getWaterColor();
    m_waterAlpha = config.getWaterAlpha();
    m_windStrength = config.getWindStrength();
    m_windDirection = config.getWindDirection();

    m_curtain.width = config.getCurtainWidth();
    m_curtain.height = config.getCurtainHeight();
    float duration = config.getCurtainFallDuration();
    m_curtain.offsetSpeed = (duration > 0) ? 1.0f / duration : 0.5f;
    m_curtain.offset = 0.0f;
    m_curtain.currentIndex = 0;
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    m_initialized = true;
    qDebug() << "OpenGL Version:" << glGetString(GL_VERSION);

    setupShaders();
    setupBuffers();

    // 强制所有阀门启用，形成直线水墙
    int total = m_valveGridWidth * m_valveGridHeight;
    m_valveStates.resize(total);
    for (int i = 0; i < total; ++i) {
        m_valveStates[i] = { true, 1.0f };
    }

    createValveGrid();
    initCurtain();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.02f, 0.03f, 0.08f, 1.0f);

    autoAdjustCamera();
    m_elapsedTimer.start();
    m_lastTime = 0;
    m_timerId = startTimer(16);
}

void GLWidget::createValveGrid()
{
    m_fountains.clear();
    m_waterJets.clear();

    float totalW = m_valveGridWidth * m_valveSpacing;
    float totalD = m_valveGridHeight * m_valveSpacing;
    m_valveStartX = -totalW / 2.0f;
    m_valveStartZ = -totalD / 2.0f;

    for (int row = 0; row < m_valveGridHeight; ++row) {
        for (int col = 0; col < m_valveGridWidth; ++col) {
            int idx = row * m_valveGridWidth + col;
            if (!m_valveStates[idx].enabled) continue;
            float x = m_valveStartX + col * m_valveSpacing;
            float z = m_valveStartZ + row * m_valveSpacing;

            FountainInfo info;
            info.position = QVector3D(x, m_valveBaseHeight, z);
            info.sprayStrength = 12.0f;
            info.sprayAngle = 85.0f * M_PI / 180.0f; // 几乎垂直向上，但水柱向下？喷射方向需要调整
            // 为了向下流水，我们让水柱竖直向下
            info.sprayAngle = -85.0f * M_PI / 180.0f; // 向下喷射
            info.sprayDirection = 0.0f;
            info.rotationAngle = 0.0f;
            m_fountains.append(info);

            WaterJetSegment jet;
            jet.start = QVector3D(x, m_valveBaseHeight + 0.2f, z);
            // 水柱向下延伸
            jet.end = jet.start + QVector3D(0.0f, -m_valveMaxLength, 0.0f);
            jet.width = 0.06f;
            jet.life = 0.02f;
            m_waterJets.append(jet);
        }
    }
    qDebug() << "Created" << m_fountains.size() << "active valves";
}

void GLWidget::updateValveWaterJets(float dt)
{
    static float timeOffset = 0.0f;
    timeOffset += dt;
    auto& music = MusicPlayer::getInstance();

    int jetIdx = 0;
    for (int row = 0; row < m_valveGridHeight; ++row) {
        for (int col = 0; col < m_valveGridWidth; ++col) {
            int idx = row * m_valveGridWidth + col;
            if (!m_valveStates[idx].enabled) continue;
            auto& jet = m_waterJets[jetIdx];
            const auto& fountain = m_fountains[jetIdx];
            jet.start = QVector3D(fountain.position.x(), fountain.position.y() + 0.2f, fountain.position.z());

            float length = m_valveMaxLength * m_valveStates[idx].intensity;
            if (m_musicSyncEnabled) {
                float beat = music.getBeatStrength();
                length *= (0.6f + beat * 0.8f);
            }
            length += sin(timeOffset * 2.5f + jetIdx) * 0.05f;
            if (length < 0.05f) length = 0.0f;

            // 向下延伸
            jet.end = jet.start + QVector3D(0.0f, -length, 0.0f);
            jet.width = 0.06f * (0.5f + m_valveStates[idx].intensity * 0.5f);
            jet.life = 0.02f;
            jetIdx++;
        }
    }
}

void GLWidget::createParticle(int fountainId)
{
    if (m_particles.size() >= MAX_PARTICLES) return;
    if (fountainId < 0 || fountainId >= m_waterJets.size()) return;
    const auto& jet = m_waterJets[fountainId];
    if ((jet.end - jet.start).length() < 0.1f) return;

    Particle p;
    p.life = randomRange(m_particleMinLife, m_particleMaxLife);
    p.size = randomRange(m_particleMinSize, m_particleMaxSize);
    // 粒子从水柱末端产生
    p.pos = jet.end + QVector3D(randomRange(-0.08f, 0.08f), -0.05f, randomRange(-0.08f, 0.08f));
    p.vel = QVector3D(randomRange(-m_particleSpeedX, m_particleSpeedX),
        randomRange(m_particleSpeedYMin, m_particleSpeedYMax),
        randomRange(-m_particleSpeedZ, m_particleSpeedZ));
    p.acc = QVector3D(0.0f, m_gravity, 0.0f);
    m_particles.append(p);
}

void GLWidget::updateParticles(float dt)
{
    dt = qMin(dt, 0.033f);
    m_spawnTimer += dt;
    while (m_spawnTimer >= m_spawnRate && m_particles.size() < MAX_PARTICLES - 100) {
        m_spawnTimer -= m_spawnRate;
        int toCreate = QRandomGenerator::global()->bounded(2, 5);
        for (int k = 0; k < toCreate; ++k) {
            if (m_waterJets.empty()) break;
            int id = QRandomGenerator::global()->bounded(m_waterJets.size());
            createParticle(id);
        }
    }

    for (int i = 0; i < m_particles.size(); ++i) {
        auto& p = m_particles[i];
        p.vel += p.acc * dt;
        p.pos += p.vel * dt;
        p.life -= dt * 0.35f;
        p.vel *= 0.998f;
        if (p.pos.y() <= GROUND_Y) {
            m_particles.removeAt(i);
            i--;
        }
        else if (fabs(p.pos.x()) > m_poolWidth / 2 + 1.0f ||
            fabs(p.pos.z()) > m_poolDepth / 2 + 1.0f ||
            p.life <= 0.0f) {
            m_particles.removeAt(i);
            i--;
        }
    }
}

void GLWidget::initCurtain()
{
    auto& config = ConfigManager::getInstance();
    m_curtain.width = config.getCurtainWidth();
    m_curtain.height = config.getCurtainHeight();
    m_curtain.position = QVector3D(0.0f, 1.0f, -4.0f);
    loadCurtainImages(config.getCurtainImagePath());
    updateCurtainTexture();
}

void GLWidget::loadCurtainImages(const QString& folderPath)
{
    m_curtain.images.clear();
    QDir dir(folderPath);
    QStringList filters;
    filters << "*.bmp" << "*.BMP";
    QFileInfoList list = dir.entryInfoList(filters);
    for (const auto& info : list) {
        QImage img;
        if (img.load(info.absoluteFilePath())) {
            m_curtain.images.append(img.convertToFormat(QImage::Format_RGBA8888));
        }
    }
    if (m_curtain.images.isEmpty()) {
        QImage defaultImg(512, 512, QImage::Format_RGBA8888);
        defaultImg.fill(Qt::blue);
        m_curtain.images.append(defaultImg);
    }
    m_curtain.currentIndex = 0;
    updateCurtainTexture();
}

void GLWidget::updateCurtainTexture()
{
    if (m_curtain.textureId == 0) glGenTextures(1, &m_curtain.textureId);
    glBindTexture(GL_TEXTURE_2D, m_curtain.textureId);
    const QImage& img = m_curtain.images[m_curtain.currentIndex];
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void GLWidget::renderCurtain(const QMatrix4x4& mvp)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_curtainProgram.bind();
    m_curtainProgram.setUniformValue(m_curtainUniformMVP, mvp);
    m_curtainProgram.setUniformValue(m_curtainUniformTex, 0);
    m_curtainProgram.setUniformValue(m_curtainUniformOffset, m_curtain.offset);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_curtain.textureId);
    glBindVertexArray(m_curtainVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    m_curtainProgram.release();
}

void GLWidget::setupShaders()
{
    const char* particleVS = R"(
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
    const char* particleFS = R"(
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
    const char* jetVS = R"(
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
    const char* jetFS = R"(
        #version 330 core
        in vec2 vTexCoord;
        out vec4 FragColor;
        uniform float uTime;
        void main() {
            float flow = fract(vTexCoord.y * 5.0 - uTime * 1.2);
            float stripe = smoothstep(0.3, 0.7, flow) * 0.15;
            vec3 color = vec3(0.3, 0.72, 0.98);
            color += vec3(0.2, 0.2, 0.15) * stripe;
            float edge = 1.0 - abs(vTexCoord.x - 0.5) * 1.2;
            float alpha = 0.65 * edge;
            FragColor = vec4(color, alpha);
        }
    )";
    const char* poolVS = R"(
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
    QString poolFS = QString(R"(
        #version 330 core
        in vec2 vTexCoord;
        out vec4 FragColor;
        void main() {
            vec3 color = vec3(%1, %2, %3);
            float alpha = %4;
            FragColor = vec4(color, alpha);
        }
    )").arg(m_waterColor.x()).arg(m_waterColor.y()).arg(m_waterColor.z()).arg(m_waterAlpha);
    const char* curtainVS = R"(
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
    const char* curtainFS = R"(
        #version 330 core
        in vec2 vTexCoord;
        out vec4 FragColor;
        uniform sampler2D uTexture;
        uniform float uOffset;
        void main() {
            vec2 uv = vec2(vTexCoord.x, vTexCoord.y + uOffset);
            if (uv.y > 1.0) discard;
            FragColor = texture(uTexture, uv);
        }
    )";

    m_particleProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, particleVS);
    m_particleProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, particleFS);
    m_particleProgram.link();

    m_jetProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, jetVS);
    m_jetProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, jetFS);
    m_jetProgram.link();

    m_poolProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, poolVS);
    m_poolProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, poolFS.toStdString().c_str());
    m_poolProgram.link();

    m_curtainProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, curtainVS);
    m_curtainProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, curtainFS);
    m_curtainProgram.link();

    m_uniformMVP = m_particleProgram.uniformLocation("uMVP");
    m_uniformTime = m_particleProgram.uniformLocation("uTime");
    m_curtainUniformMVP = m_curtainProgram.uniformLocation("uMVP");
    m_curtainUniformTex = m_curtainProgram.uniformLocation("uTexture");
    m_curtainUniformOffset = m_curtainProgram.uniformLocation("uOffset");
}

void GLWidget::setupBuffers()
{
    glGenVertexArrays(1, &m_particleVAO);
    glBindVertexArray(m_particleVAO);
    glGenBuffers(1, &m_particleVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_particleVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 6 * 5 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glGenVertexArrays(1, &m_jetVAO);
    glBindVertexArray(m_jetVAO);
    glGenBuffers(1, &m_jetVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_jetVBO);
    glBufferData(GL_ARRAY_BUFFER, m_valveGridWidth * m_valveGridHeight * 6 * 5 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    float y = -1.05f;
    float poolVerts[] = {
        -m_poolWidth / 2, y, -m_poolDepth / 2, 0,0,
         m_poolWidth / 2, y, -m_poolDepth / 2, 1,0,
        -m_poolWidth / 2, y,  m_poolDepth / 2, 0,1,
         m_poolWidth / 2, y, -m_poolDepth / 2, 1,0,
         m_poolWidth / 2, y,  m_poolDepth / 2, 1,1,
        -m_poolWidth / 2, y,  m_poolDepth / 2, 0,1
    };
    glGenVertexArrays(1, &m_poolVAO);
    glBindVertexArray(m_poolVAO);
    glGenBuffers(1, &m_poolVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_poolVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(poolVerts), poolVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    float halfW = m_curtain.width / 2.0f;
    float halfH = m_curtain.height / 2.0f;
    float yb = m_curtain.position.y();
    float yt = yb + m_curtain.height;
    float xl = m_curtain.position.x() - halfW;
    float xr = m_curtain.position.x() + halfW;
    float z = m_curtain.position.z();
    float curtainVerts[] = {
        xl, yb, z, 0,0,
        xr, yb, z, 1,0,
        xr, yt, z, 1,1,
        xl, yt, z, 0,1
    };
    glGenVertexArrays(1, &m_curtainVAO);
    glGenBuffers(1, &m_curtainVBO);
    glBindVertexArray(m_curtainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_curtainVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(curtainVerts), curtainVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void GLWidget::paintGL()
{
    float now = m_elapsedTimer.elapsed() / 1000.0f;
    float dt = qMin(0.033f, now - m_lastTime);
    if (dt > 0.001f) {
        m_lastTime = now;
        updateValveWaterJets(dt);
        updateParticles(dt);
        if (!m_curtain.images.isEmpty()) {
            m_curtain.offset += dt * m_curtain.offsetSpeed;
            if (m_curtain.offset >= 1.0f) {
                m_curtain.offset = 0.0f;
                m_curtain.currentIndex = (m_curtain.currentIndex + 1) % m_curtain.images.size();
                updateCurtainTexture();
            }
        }
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    QMatrix4x4 view;
    view.lookAt(m_cameraPos, m_cameraTarget, QVector3D(0, 1, 0));
    QMatrix4x4 mvp = m_projection * view;

    m_poolProgram.bind();
    m_poolProgram.setUniformValue(m_uniformMVP, mvp);
    glBindVertexArray(m_poolVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    m_poolProgram.release();

    renderCurtain(mvp);

    if (!m_waterJets.empty()) {
        std::vector<float> jetVerts;
        for (const auto& jet : m_waterJets) {
            float tw = jet.width;
            float bw = jet.life;
            QVector3D s = jet.start, e = jet.end;
            jetVerts.push_back(s.x() - tw); jetVerts.push_back(s.y()); jetVerts.push_back(s.z()); jetVerts.push_back(0); jetVerts.push_back(0);
            jetVerts.push_back(s.x() + tw); jetVerts.push_back(s.y()); jetVerts.push_back(s.z()); jetVerts.push_back(1); jetVerts.push_back(0);
            jetVerts.push_back(e.x() - bw); jetVerts.push_back(e.y()); jetVerts.push_back(e.z()); jetVerts.push_back(0); jetVerts.push_back(1);
            jetVerts.push_back(s.x() + tw); jetVerts.push_back(s.y()); jetVerts.push_back(s.z()); jetVerts.push_back(1); jetVerts.push_back(0);
            jetVerts.push_back(e.x() + bw); jetVerts.push_back(e.y()); jetVerts.push_back(e.z()); jetVerts.push_back(1); jetVerts.push_back(1);
            jetVerts.push_back(e.x() - bw); jetVerts.push_back(e.y()); jetVerts.push_back(e.z()); jetVerts.push_back(0); jetVerts.push_back(1);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_jetVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, jetVerts.size() * sizeof(float), jetVerts.data());
        m_jetProgram.bind();
        m_jetProgram.setUniformValue(m_uniformMVP, mvp);
        m_jetProgram.setUniformValue(m_uniformTime, now);
        glBindVertexArray(m_jetVAO);
        glDrawArrays(GL_TRIANGLES, 0, jetVerts.size() / 5);
        glBindVertexArray(0);
        m_jetProgram.release();
    }

    if (!m_particles.empty()) {
        std::vector<float> pVerts;
        for (const auto& p : m_particles) {
            float sz = p.size * (0.5f + p.life * 0.8f);
            float x = p.pos.x(), y = p.pos.y(), z = p.pos.z();
            pVerts.push_back(x - sz); pVerts.push_back(y - sz); pVerts.push_back(z); pVerts.push_back(0); pVerts.push_back(0);
            pVerts.push_back(x + sz); pVerts.push_back(y - sz); pVerts.push_back(z); pVerts.push_back(1); pVerts.push_back(0);
            pVerts.push_back(x - sz); pVerts.push_back(y + sz); pVerts.push_back(z); pVerts.push_back(0); pVerts.push_back(1);
            pVerts.push_back(x + sz); pVerts.push_back(y - sz); pVerts.push_back(z); pVerts.push_back(1); pVerts.push_back(0);
            pVerts.push_back(x + sz); pVerts.push_back(y + sz); pVerts.push_back(z); pVerts.push_back(1); pVerts.push_back(1);
            pVerts.push_back(x - sz); pVerts.push_back(y + sz); pVerts.push_back(z); pVerts.push_back(0); pVerts.push_back(1);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_particleVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, pVerts.size() * sizeof(float), pVerts.data());
        m_particleProgram.bind();
        m_particleProgram.setUniformValue(m_uniformMVP, mvp);
        m_particleProgram.setUniformValue(m_uniformTime, now);
        glBindVertexArray(m_particleVAO);
        glDrawArrays(GL_TRIANGLES, 0, pVerts.size() / 5);
        glBindVertexArray(0);
        m_particleProgram.release();
    }
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    float aspect = float(w) / float(h);
    m_projection.setToIdentity();
    m_projection.perspective(45.0f, aspect, 0.1f, 100.0f);
}

void GLWidget::timerEvent(QTimerEvent*) { update(); }

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
        for (int i = 0; i < 100; ++i) {
            if (m_waterJets.empty()) break;
            int id = QRandomGenerator::global()->bounded(m_waterJets.size());
            createParticle(id);
        }
        break;
    case Qt::Key_Up:
        m_spawnRate = qMax(0.005f, m_spawnRate - 0.002f);
        break;
    case Qt::Key_Down:
        m_spawnRate = qMin(0.05f, m_spawnRate + 0.002f);
        break;
    case Qt::Key_R:
        m_particles.clear();
        break;
    default:
        break;
    }
}

void GLWidget::autoAdjustCamera()
{
    float totalW = m_valveGridWidth * m_valveSpacing;
    float required = totalW / 1.2f;
    m_cameraDistance = qBound(10.0f, required, 25.0f);
    m_cameraTarget = QVector3D(0.0f, 1.5f, 0.0f);
    float radX = qDegreesToRadians(m_cameraAngleX);
    float radY = qDegreesToRadians(m_cameraAngleY);
    m_cameraPos.setX(m_cameraDistance * cos(radY) * sin(radX));
    m_cameraPos.setY(m_cameraDistance * sin(radY));
    m_cameraPos.setZ(m_cameraDistance * cos(radY) * cos(radX));
}

// 兼容旧接口
void GLWidget::updateFountainsFromData(const QVector<FountainData>& fountains)
{
    Q_UNUSED(fountains);
    qDebug() << "updateFountainsFromData called but ignored (using valve array from image)";
}

// 音乐同步接口
void GLWidget::setMusicSyncEnabled(bool enabled) { m_musicSyncEnabled = enabled; }
void GLWidget::setMusicSensitivity(float sensitivity) { m_musicSensitivity = qBound(0.3f, sensitivity, 2.5f); }
void GLWidget::loadMusicFile(const QString& filePath) { MusicPlayer::getInstance().loadMusic(filePath); }
void GLWidget::playMusic() { MusicPlayer::getInstance().play(); }
void GLWidget::pauseMusic() { MusicPlayer::getInstance().pause(); }
void GLWidget::stopMusic() { MusicPlayer::getInstance().stop(); }