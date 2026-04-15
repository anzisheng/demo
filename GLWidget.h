#pragma once

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QVector>
#include <QElapsedTimer>
#include <QMatrix4x4>

struct Particle {
    QVector3D pos;
    QVector3D vel;
    QVector3D acc;
    float life;
    float size;
};

struct FountainInfo {
    QVector3D position;
    float sprayStrength;
    float sprayAngle;
    float sprayDirection;
    float rotationAngle;
};

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
    void keyPressEvent(QKeyEvent* event) override;

private:
    void updateParticles(float deltaTime);
    void updateWaterJets(float deltaTime);
    void setupBuffers();
    void setupShaders();
    void createFountains();
    void createParticle(int fountainId);

    // 相机控制
    QMatrix4x4 m_projection;
    QVector3D m_cameraPos;
    QVector3D m_cameraTarget;
    float m_cameraDistance;
    float m_cameraAngleX;
    float m_cameraAngleY;
    QPoint m_lastMousePos;
    bool m_mousePressed;

    // 着色器
    QOpenGLShaderProgram m_particleProgram;
    QOpenGLShaderProgram m_jetProgram;
    QOpenGLShaderProgram m_valveProgram;
    QOpenGLShaderProgram m_poolProgram;

    // 缓冲区
    GLuint m_particleVAO = 0;
    GLuint m_particleVBO = 0;
    GLuint m_jetVAO = 0;
    GLuint m_jetVBO = 0;
    GLuint m_valveVAO = 0;
    GLuint m_poolVAO = 0;
    GLuint m_poolVBO = 0;

    // 数据
    QVector<Particle> m_particles;
    QVector<FountainInfo> m_fountains;
    QVector<WaterJetSegment> m_waterJets;

    int m_uniformMVP = 0;
    int m_uniformTime = 0;

    static const int FOUNTAIN_COUNT = 50;
    static const int MAX_PARTICLES = 10000;
    static const float GRAVITY;
    static const float GROUND_Y;

    float m_spawnTimer;
    float m_spawnRate;

    QElapsedTimer m_elapsedTimer;
    float m_lastTime = 0;
    int m_timerId = 0;
};