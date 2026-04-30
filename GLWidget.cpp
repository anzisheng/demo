#include "GLWidget.h"
#include "ConfigManager.h"
#include <QMatrix4x4>
#include <QRandomGenerator>
#include <QDebug>
#include <cmath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QTimer>

inline float randomRange(float min, float max) {
    return min + QRandomGenerator::global()->generateDouble() * (max - min);
}

GLWidget::GLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_cameraPos(0.0f, 5.0f, 18.0f)
    , m_cameraTarget(0.0f, 1.5f, 0.0f)
    , m_cameraDistance(18.0f)
    , m_cameraAngleX(0.0f)
    , m_cameraAngleY(35.0f)
    , m_mousePressed(false)
    , m_valveCount(200)
    , m_valveSpacing(0.25f)
    , m_valveBaseHeight(3.0f)
    , m_valveSize(0.2f)
    , m_dropBurstInterval(0.03f)
    , m_dropMinSize(0.05f)
    , m_dropMaxSize(0.09f)
    , m_dropMinLife(1.5f)
    , m_dropMaxLife(2.0f)
    , m_dropSpeedYMin(-14.0f)
    , m_dropSpeedYMax(-11.0f)
    , m_gravity(-9.8f)
    , m_poolWidth(20.0f)
    , m_poolDepth(12.0f)
    , m_waterColor(0.2f, 0.65f, 0.95f)
    , m_waterAlpha(0.65f)
    , m_burstTimer(0.0f)
    , m_lastTime(0.0f)
    , m_timerId(0)
    , m_initialized(false)
{
    ConfigManager::getInstance().loadConfig();
    applyConfig();
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    m_drops.reserve(m_maxDrops);
}

GLWidget::~GLWidget()
{
    makeCurrent();
    if (m_dropVBO) glDeleteBuffers(1, &m_dropVBO);
    if (m_dropVAO) glDeleteVertexArrays(1, &m_dropVAO);
    if (m_valveVBO) glDeleteBuffers(1, &m_valveVBO);
    if (m_valveVAO) glDeleteVertexArrays(1, &m_valveVAO);
    if (m_poolVBO) glDeleteBuffers(1, &m_poolVBO);
    if (m_poolVAO) glDeleteVertexArrays(1, &m_poolVAO);
    doneCurrent();
}

void GLWidget::applyConfig()
{
    auto& config = ConfigManager::getInstance();
    m_valveCount = config.getWaterValveCount();
    m_valveSpacing = config.getWaterValveSpacing();
    m_valveBaseHeight = config.getWaterValveBaseHeight();
    m_valveSize = config.getWaterValveSize();

    m_dropBurstInterval = config.getDropBurstInterval();
    m_dropMinSize = config.getDropMinSize();
    m_dropMaxSize = config.getDropMaxSize();
    m_dropMinLife = config.getDropMinLife();
    m_dropMaxLife = config.getDropMaxLife();
    m_dropSpeedYMin = config.getDropSpeedYMin();
    m_dropSpeedYMax = config.getDropSpeedYMax();
    m_gravity = config.getGravity();

    m_poolWidth = config.getPoolWidth();
    m_poolDepth = config.getPoolDepth();
    m_waterColor = config.getWaterColor();
    m_waterAlpha = config.getWaterAlpha();
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    m_initialized = true;
    qDebug() << "OpenGL:" << glGetString(GL_VERSION);

    setupShaders();
    setupBuffers();

    createValves();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.02f, 0.03f, 0.08f, 1.0f);

    autoAdjustCamera();
    m_elapsedTimer.start();
    m_lastTime = 0;
    m_timerId = startTimer(16);
}

void GLWidget::createValves()
{
    m_valvePositions.clear();
    float startX = -(m_valveCount - 1) * m_valveSpacing / 2.0f;
    for (int i = 0; i < m_valveCount; ++i) {
        float x = startX + i * m_valveSpacing;
        QVector3D pos(x, m_valveBaseHeight, 0.0f);
        m_valvePositions.append(pos);
    }

    // 构建立方体顶点数据
    std::vector<float> valveVertices;
    float s = m_valveSize / 2.0f;
    for (const auto& pos : m_valvePositions) {
        float x = pos.x(), y = pos.y(), z = pos.z();
        float verts[] = {
            // 前面
            x - s, y - s, z + s,  x + s, y - s, z + s,  x + s, y + s, z + s,
            x - s, y - s, z + s,  x + s, y + s, z + s,  x - s, y + s, z + s,
            // 后面
            x - s, y - s, z - s,  x - s, y + s, z - s,  x + s, y + s, z - s,
            x - s, y - s, z - s,  x + s, y + s, z - s,  x + s, y - s, z - s,
            // 左面
            x - s, y - s, z - s,  x - s, y + s, z - s,  x - s, y + s, z + s,
            x - s, y - s, z - s,  x - s, y + s, z + s,  x - s, y - s, z + s,
            // 右面
            x + s, y - s, z - s,  x + s, y + s, z + s,  x + s, y + s, z - s,
            x + s, y - s, z - s,  x + s, y - s, z + s,  x + s, y + s, z + s,
            // 上面
            x - s, y + s, z - s,  x - s, y + s, z + s,  x + s, y + s, z + s,
            x - s, y + s, z - s,  x + s, y + s, z + s,  x + s, y + s, z - s,
            // 下面
            x - s, y - s, z - s,  x + s, y - s, z - s,  x + s, y - s, z + s,
            x - s, y - s, z - s,  x + s, y - s, z + s,  x - s, y - s, z + s
        };
        for (float v : verts) valveVertices.push_back(v);
    }
    m_valveVertexCount = valveVertices.size() / 3;

    glBindVertexArray(m_valveVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_valveVBO);
    glBufferData(GL_ARRAY_BUFFER, valveVertices.size() * sizeof(float), valveVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    qDebug() << "Created" << m_valveCount << "valves";
}

// 为所有水阀各创建一个水滴
void GLWidget::createDropForAllValves()
{
    if (m_drops.size() + m_valveCount > m_maxDrops) return;
    for (int i = 0; i < m_valveCount; ++i) {
        const auto& pos = m_valvePositions[i];
        Drop d;
        d.life = randomRange(m_dropMinLife, m_dropMaxLife);
        d.size = randomRange(m_dropMinSize, m_dropMaxSize);
        d.pos = QVector3D(pos.x(), pos.y() - 0.1f, pos.z());
        // 垂直速度，无水平分量
        d.vel = QVector3D(0.0f, randomRange(m_dropSpeedYMin, m_dropSpeedYMax), 0.0f);
        d.acc = QVector3D(0.0f, m_gravity, 0.0f);
        m_drops.append(d);
    }
}

void GLWidget::updateDrops(float dt)
{
    dt = qMin(dt, 0.033f);

    // 批量同步生成水滴：每隔固定时间，所有水阀同时产生一个水滴
    m_burstTimer += dt;
    while (m_burstTimer >= m_dropBurstInterval && m_drops.size() < m_maxDrops - m_valveCount) {
        m_burstTimer -= m_dropBurstInterval;
        createDropForAllValves();
    }

    // 更新所有水滴的物理状态
    for (int i = 0; i < m_drops.size(); ++i) {
        auto& d = m_drops[i];
        d.vel += d.acc * dt;
        d.pos += d.vel * dt;
        d.life -= dt * 0.5f;

        // 落地或生命周期结束则移除
        if (d.pos.y() <= -1.0f || d.life <= 0.0f) {
            m_drops.removeAt(i);
            i--;
        }
        // 边界移除（水池外）
        else if (fabs(d.pos.x()) > m_poolWidth / 2 + 1.0f ||
            fabs(d.pos.z()) > m_poolDepth / 2 + 1.0f) {
            m_drops.removeAt(i);
            i--;
        }
    }
}

void GLWidget::setupShaders()
{
    // 水滴着色器（圆形粒子）
    const char* dropVS = R"(
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
    const char* dropFS = R"(
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

    // 水阀着色器（古铜色）
    const char* valveVS = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        uniform mat4 uMVP;
        void main() {
            gl_Position = uMVP * vec4(aPos, 1.0);
        }
    )";
    const char* valveFS = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(0.7, 0.5, 0.3, 1.0);
        }
    )";

    // 水池着色器（半透明蓝）
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

    m_dropProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, dropVS);
    m_dropProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, dropFS);
    m_dropProgram.link();

    m_valveProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, valveVS);
    m_valveProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, valveFS);
    m_valveProgram.link();

    m_poolProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, poolVS);
    m_poolProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, poolFS.toStdString().c_str());
    m_poolProgram.link();

    m_uniformMVP = m_dropProgram.uniformLocation("uMVP");
    m_uniformTime = m_dropProgram.uniformLocation("uTime");
    m_valveUniformMVP = m_valveProgram.uniformLocation("uMVP");
}

void GLWidget::setupBuffers()
{
    // 水滴缓冲区
    glGenVertexArrays(1, &m_dropVAO);
    glBindVertexArray(m_dropVAO);
    glGenBuffers(1, &m_dropVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_dropVBO);
    glBufferData(GL_ARRAY_BUFFER, m_maxDrops * 6 * 5 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    // 水阀模型
    glGenVertexArrays(1, &m_valveVAO);
    glGenBuffers(1, &m_valveVBO);

    // 水池平面
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
}

void GLWidget::paintGL()
{
    float now = m_elapsedTimer.elapsed() / 1000.0f;
    float dt = qMin(0.033f, now - m_lastTime);
    if (dt > 0.001f) {
        m_lastTime = now;
        updateDrops(dt);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    QMatrix4x4 view;
    view.lookAt(m_cameraPos, m_cameraTarget, QVector3D(0, 1, 0));
    QMatrix4x4 mvp = m_projection * view;

    // 水池
    m_poolProgram.bind();
    m_poolProgram.setUniformValue(m_uniformMVP, mvp);
    glBindVertexArray(m_poolVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    m_poolProgram.release();

    // 水阀
    m_valveProgram.bind();
    m_valveProgram.setUniformValue(m_valveUniformMVP, mvp);
    glBindVertexArray(m_valveVAO);
    glDrawArrays(GL_TRIANGLES, 0, m_valveVertexCount);
    glBindVertexArray(0);
    m_valveProgram.release();

    // 水滴粒子
    if (!m_drops.empty()) {
        std::vector<float> vertices;
        vertices.reserve(m_drops.size() * 6 * 5);
        for (const auto& d : m_drops) {
            float sz = d.size * (0.5f + d.life * 0.8f);
            float x = d.pos.x(), y = d.pos.y(), z = d.pos.z();
            vertices.push_back(x - sz); vertices.push_back(y - sz); vertices.push_back(z); vertices.push_back(0); vertices.push_back(0);
            vertices.push_back(x + sz); vertices.push_back(y - sz); vertices.push_back(z); vertices.push_back(1); vertices.push_back(0);
            vertices.push_back(x - sz); vertices.push_back(y + sz); vertices.push_back(z); vertices.push_back(0); vertices.push_back(1);
            vertices.push_back(x + sz); vertices.push_back(y - sz); vertices.push_back(z); vertices.push_back(1); vertices.push_back(0);
            vertices.push_back(x + sz); vertices.push_back(y + sz); vertices.push_back(z); vertices.push_back(1); vertices.push_back(1);
            vertices.push_back(x - sz); vertices.push_back(y + sz); vertices.push_back(z); vertices.push_back(0); vertices.push_back(1);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_dropVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
        m_dropProgram.bind();
        m_dropProgram.setUniformValue(m_uniformMVP, mvp);
        m_dropProgram.setUniformValue(m_uniformTime, now);
        glBindVertexArray(m_dropVAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 5);
        glBindVertexArray(0);
        m_dropProgram.release();
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
    m_cameraDistance = qBound(6.0f, m_cameraDistance, 30.0f);
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
    if (event->button() == Qt::LeftButton) m_mousePressed = false;
}

void GLWidget::autoAdjustCamera()
{
    float totalW = (m_valveCount - 1) * m_valveSpacing;
    float required = totalW / 1.2f;
    m_cameraDistance = qBound(10.0f, required, 28.0f);
    // 目标点设为水阀中间偏下
    m_cameraTarget = QVector3D(0.0f, m_valveBaseHeight - 1.0f, 0.0f);
    // 相机高度相对于水阀位置
    float cameraHeight = m_valveBaseHeight + 5.0f;
    float radX = qDegreesToRadians(m_cameraAngleX);
    float radY = qDegreesToRadians(m_cameraAngleY);
    m_cameraPos.setX(m_cameraDistance * cos(radY) * sin(radX));
    m_cameraPos.setY(cameraHeight);
    m_cameraPos.setZ(m_cameraDistance * cos(radY) * cos(radX));
}