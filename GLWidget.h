#pragma once

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QVector>
#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QImage>
#include "FountainData.h"

struct Particle { QVector3D pos; QVector3D vel; QVector3D acc; float life; float size; };
struct FountainInfo { QVector3D position; float sprayStrength; float sprayAngle; float sprayDirection; float rotationAngle; };
struct WaterJetSegment { QVector3D start; QVector3D end; float width; float life; };

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core
{
    Q_OBJECT
public:
    explicit GLWidget(QWidget* parent = nullptr);
    ~GLWidget();
    void setMusicSyncEnabled(bool enabled);
    void setMusicSensitivity(float sensitivity);
    void loadMusicFile(const QString& filePath);
    void playMusic();
    void pauseMusic();
    void stopMusic();
    void updateFountainsFromData(const QVector<FountainData>& fountains);
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
    void loadValveControlImage(const QString& filePath);
    void createValveGrid();
    void updateValveWaterJets(float dt);
    void createParticle(int fountainId);
    void updateParticles(float dt);
    void setupBuffers();
    void setupShaders();
    void applyConfig();
    void autoAdjustCamera();
    void initCurtain();
    void loadCurtainImages(const QString& folderPath);
    void updateCurtainTexture();
    void renderCurtain(const QMatrix4x4& mvp);
    // 相机
    QMatrix4x4 m_projection;
    QVector3D m_cameraPos, m_cameraTarget;
    float m_cameraDistance, m_cameraAngleX, m_cameraAngleY;
    QPoint m_lastMousePos;
    bool m_mousePressed;
    // 着色器
    QOpenGLShaderProgram m_particleProgram, m_jetProgram, m_poolProgram, m_curtainProgram;
    // 缓冲区
    GLuint m_particleVAO = 0, m_particleVBO = 0;
    GLuint m_jetVAO = 0, m_jetVBO = 0;
    GLuint m_poolVAO = 0, m_poolVBO = 0;
    GLuint m_curtainVAO = 0, m_curtainVBO = 0;
    // 数据
    QVector<Particle> m_particles;
    QVector<FountainInfo> m_fountains;
    QVector<WaterJetSegment> m_waterJets;
    // 水阀状态
    struct ValveState { bool enabled; float intensity; };
    QVector<ValveState> m_valveStates;
    int m_valveGridWidth, m_valveGridHeight;
    float m_valveSpacing, m_valveStartX, m_valveStartZ, m_valveBaseHeight, m_valveMaxLength;
    // 水帘
    struct CurtainInfo {
        QVector3D position;
        float width, height;
        GLuint textureId = 0;
        QVector<QImage> images;
        int currentIndex = 0;
        float offset = 0.0f, offsetSpeed = 0.0f;
    } m_curtain;
    int m_uniformMVP = 0, m_uniformTime = 0;
    int m_curtainUniformMVP = 0, m_curtainUniformTex = 0, m_curtainUniformOffset = 0;
    static const int MAX_PARTICLES = 10000;
    static const float GROUND_Y;
    // 配置参数
    float m_spawnRate, m_particleMinSize, m_particleMaxSize, m_particleMinLife, m_particleMaxLife;
    float m_particleSpeedX, m_particleSpeedYMin, m_particleSpeedYMax, m_particleSpeedZ, m_gravity;
    float m_poolWidth, m_poolDepth, m_waterAlpha, m_windStrength, m_windDirection;
    QVector3D m_waterColor;
    bool m_musicSyncEnabled;
    float m_musicSensitivity;
    float m_spawnTimer, m_lastTime;
    int m_timerId;
    bool m_initialized;
    QElapsedTimer m_elapsedTimer;
};