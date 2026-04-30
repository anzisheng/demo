#pragma once

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QVector>
#include <QElapsedTimer>
#include <QMatrix4x4>

struct WaterJetSegment {
    QVector3D start;
    QVector3D end;
    float width;
    float life;
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
    void createValveLine();
    void updateWaterJets(float dt);
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
    QOpenGLShaderProgram m_jetProgram;
    QOpenGLShaderProgram m_poolProgram;
    QOpenGLShaderProgram m_valveProgram;

    // 缓冲区
    GLuint m_jetVAO = 0, m_jetVBO = 0;
    GLuint m_poolVAO = 0, m_poolVBO = 0;
    GLuint m_valveVAO = 0, m_valveVBO = 0;

    QVector<WaterJetSegment> m_waterJets;
    QVector<QVector3D> m_valvePositions;
    int m_valveVertexCount = 0;

    // 参数
    int m_valveCount;
    float m_valveSpacing, m_valveBaseHeight, m_valveMaxLength, m_valveSize;
    float m_poolWidth, m_poolDepth, m_waterAlpha;
    QVector3D m_waterColor;

    int m_uniformMVP = 0;
    int m_uniformTime = 0;
    int m_valveUniformMVP = 0;

    float m_lastTime = 0;
    int m_timerId = 0;
    bool m_initialized = false;

    QElapsedTimer m_elapsedTimer;
};