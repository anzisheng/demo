#pragma once

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QVector>
#include <QElapsedTimer>
#include <QMatrix4x4>

struct Drop {
    QVector3D pos;
    QVector3D vel;
    QVector3D acc;
    float life;
    float size;
};

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core
{
    Q_OBJECT
public:
    explicit GLWidget(QWidget* parent = nullptr);
    ~GLWidget();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void timerEvent(QTimerEvent*) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void createValves();
    void updateDrops(float dt);
    void createDropForAllValves();   // 为所有水阀各创建一个水滴
    void setupBuffers();
    void setupShaders();
    void applyConfig();
    void autoAdjustCamera();

    // 相机
    QMatrix4x4 m_projection;
    QVector3D m_cameraPos, m_cameraTarget;
    float m_cameraDistance, m_cameraAngleX, m_cameraAngleY;
    QPoint m_lastMousePos;
    bool m_mousePressed;

    // 着色器
    QOpenGLShaderProgram m_dropProgram;
    QOpenGLShaderProgram m_valveProgram;
    QOpenGLShaderProgram m_poolProgram;

    // 缓冲区
    GLuint m_dropVAO = 0, m_dropVBO = 0;
    GLuint m_valveVAO = 0, m_valveVBO = 0;
    GLuint m_poolVAO = 0, m_poolVBO = 0;

    QVector<Drop> m_drops;
    QVector<QVector3D> m_valvePositions;
    int m_valveVertexCount = 0;

    // 参数（从配置文件读取）
    int m_valveCount;
    float m_valveSpacing;
    float m_valveBaseHeight;
    float m_valveSize;

    float m_dropBurstInterval;   // 每次批量生成水滴的间隔（秒）
    float m_dropMinSize, m_dropMaxSize;
    float m_dropMinLife, m_dropMaxLife;
    float m_dropSpeedYMin, m_dropSpeedYMax;   // 垂直速度范围，X/Z 为0
    float m_gravity;
    float m_poolWidth, m_poolDepth;
    QVector3D m_waterColor;
    float m_waterAlpha;

    // 运行时
    float m_burstTimer = 0.0f;
    float m_lastTime = 0.0f;
    int m_timerId = 0;
    bool m_initialized = false;
    int m_maxDrops = 50000;          // 最大水滴数量

    QElapsedTimer m_elapsedTimer;
    int m_uniformMVP = 0;
    int m_uniformTime = 0;
    int m_valveUniformMVP = 0;
};