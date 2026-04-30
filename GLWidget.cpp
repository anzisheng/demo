#include "GLWidget.h"
#include "ConfigManager.h"
#include <QMatrix4x4>
#include <QDebug>
#include <cmath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QTimer>
#include "GLWidget.h"
#include "ConfigManager.h"
#include <QMatrix4x4>
#include <QDebug>
#include <cmath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QTimer>

GLWidget::GLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_cameraPos(0.0f, 5.0f, 25.0f)
    , m_cameraTarget(0.0f, 2.0f, 0.0f)
    , m_cameraDistance(25.0f)
    , m_cameraAngleX(0.0f)
    , m_cameraAngleY(35.0f)
    , m_mousePressed(false)
    , m_poolWidth(50.0f)
    , m_poolDepth(8.0f)
    , m_waterAlpha(0.65f)
    , m_waterColor(0.2f, 0.65f, 0.95f)
    , m_lastTime(0.0f)
    , m_timerId(0)
    , m_initialized(false)
{
    ConfigManager::getInstance().loadConfig();
    applyConfig();
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

GLWidget::~GLWidget()
{
    makeCurrent();
    if (m_jetVBO) glDeleteBuffers(1, &m_jetVBO);
    if (m_jetVAO) glDeleteVertexArrays(1, &m_jetVAO);
    if (m_poolVBO) glDeleteBuffers(1, &m_poolVBO);
    if (m_poolVAO) glDeleteVertexArrays(1, &m_poolVAO);
    if (m_valveVBO) glDeleteBuffers(1, &m_valveVBO);
    if (m_valveVAO) glDeleteVertexArrays(1, &m_valveVAO);
    doneCurrent();
}

void GLWidget::applyConfig()
{
    auto& config = ConfigManager::getInstance();
    m_valveCount = config.getWaterValveCount();
    m_valveSpacing = config.getWaterValveSpacing();
    m_valveBaseHeight = config.getWaterValveBaseHeight();
    m_valveMaxLength = config.getWaterValveMaxLength();
    m_valveSize = config.getWaterValveSize();

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

    createValveLine();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.02f, 0.03f, 0.08f, 1.0f);

    autoAdjustCamera();
    m_elapsedTimer.start();
    m_lastTime = 0;
    m_timerId = startTimer(16);
}

void GLWidget::createValveLine()
{
    m_valvePositions.clear();
    m_waterJets.clear();

    float totalW = (m_valveCount - 1) * m_valveSpacing;
    float startX = -totalW / 2.0f;
    float z = 0.0f; // 单排，Z居中

    for (int i = 0; i < m_valveCount; ++i) {
        float x = startX + i * m_valveSpacing;
        QVector3D pos(x, m_valveBaseHeight, z);
        m_valvePositions.append(pos);

        WaterJetSegment jet;
        jet.start = QVector3D(x, m_valveBaseHeight - 0.1f, z);
        jet.end = jet.start + QVector3D(0.0f, -m_valveMaxLength, 0.0f);
        jet.width = 0.05f;
        jet.life = 0.02f;
        m_waterJets.append(jet);
    }

    // 构建水阀立方体模型
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

    qDebug() << "Created" << m_valvePositions.size() << "valves in a line";
}

void GLWidget::updateWaterJets(float dt)
{
    static float timeOffset = 0.0f;
    timeOffset += dt;
    for (int i = 0; i < m_waterJets.size(); ++i) {
        auto& jet = m_waterJets[i];
        const auto& pos = m_valvePositions[i];
        jet.start = QVector3D(pos.x(), pos.y() - 0.1f, pos.z());
        // 固定长度，加微小波动
        float length = m_valveMaxLength;
        length += sin(timeOffset * 2.0f + i) * 0.02f;
        jet.end = jet.start + QVector3D(0.0f, -length, 0.0f);
        jet.width = 0.05f;
        jet.life = 0.02f;
    }
}

void GLWidget::setupShaders()
{
    // 水柱着色器
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

    // 水池着色器
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

    // 水阀模型着色器
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

    m_jetProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, jetVS);
    m_jetProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, jetFS);
    m_jetProgram.link();

    m_poolProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, poolVS);
    m_poolProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, poolFS.toStdString().c_str());
    m_poolProgram.link();

    m_valveProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, valveVS);
    m_valveProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, valveFS);
    m_valveProgram.link();

    m_uniformMVP = m_jetProgram.uniformLocation("uMVP");
    m_uniformTime = m_jetProgram.uniformLocation("uTime");
    m_valveUniformMVP = m_valveProgram.uniformLocation("uMVP");
}

void GLWidget::setupBuffers()
{
    // 水柱缓冲区
    glGenVertexArrays(1, &m_jetVAO);
    glBindVertexArray(m_jetVAO);
    glGenBuffers(1, &m_jetVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_jetVBO);
    glBufferData(GL_ARRAY_BUFFER, m_valveCount * 6 * 5 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

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

    // 水阀模型 VAO/VBO
    glGenVertexArrays(1, &m_valveVAO);
    glGenBuffers(1, &m_valveVBO);
}

void GLWidget::paintGL()
{
    float now = m_elapsedTimer.elapsed() / 1000.0f;
    float dt = qMin(0.033f, now - m_lastTime);
    if (dt > 0.001f) {
        m_lastTime = now;
        updateWaterJets(dt);
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

    // 水柱
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

    // 水阀模型
    m_valveProgram.bind();
    m_valveProgram.setUniformValue(m_valveUniformMVP, mvp);
    glBindVertexArray(m_valveVAO);
    glDrawArrays(GL_TRIANGLES, 0, m_valveVertexCount);
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

void GLWidget::timerEvent(QTimerEvent*) { update(); }

void GLWidget::wheelEvent(QWheelEvent* event)
{
    float delta = event->angleDelta().y() / 120.0f;
    m_cameraDistance -= delta * 0.5f;
    m_cameraDistance = qBound(8.0f, m_cameraDistance, 40.0f);
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
    m_cameraDistance = qBound(12.0f, required, 40.0f);
    m_cameraTarget = QVector3D(0.0f, m_valveBaseHeight - 1.0f, 0.0f);
}