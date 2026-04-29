#pragma once

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QOpenGLShaderProgram>
#include <QVector>
#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QImage>
#include <QDir>
#include "FountainData.h"

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

    void reloadConfig();
    void updateFountainsFromData(const QVector<FountainData>& fountains);

    // 音乐同步接口
    void setMusicSyncEnabled(bool enabled);
    void setMusicSensitivity(float sensitivity);
    void loadMusicFile(const QString& filePath);
    void playMusic();
    void pauseMusic();
    void stopMusic();

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
    void applyConfig();
    void autoAdjustCamera();
    void setupGroundFountains();

    // 水帘相关
    void initCurtain();
    void loadCurtainImages(const QString& folderPath);
    void updateCurtainTexture();
    void renderCurtain(const QMatrix4x4& mvp);

    // 相机控制
    QMatrix4x4 m_projection;
    QVector3D m_cameraPos;
    QVector3D m_cameraTarget;
    float m_cameraDistance;
    float m_cameraAngleX;
    float m_cameraAngleY;
    QPoint m_lastMousePos;
    bool m_mousePressed;

    // 着色器程序
    QOpenGLShaderProgram m_particleProgram;
    QOpenGLShaderProgram m_jetProgram;
    QOpenGLShaderProgram m_valveProgram;
    QOpenGLShaderProgram m_poolProgram;
    QOpenGLShaderProgram m_curtainProgram;

    // 缓冲区对象
    GLuint m_particleVAO = 0;
    GLuint m_particleVBO = 0;
    GLuint m_jetVAO = 0;
    GLuint m_jetVBO = 0;
    GLuint m_valveVAO = 0;
    GLuint m_poolVAO = 0;
    GLuint m_poolVBO = 0;
    GLuint m_curtainVAO = 0;
    GLuint m_curtainVBO = 0;

    // 数据容器
    QVector<Particle> m_particles;
    QVector<FountainInfo> m_fountains;
    QVector<WaterJetSegment> m_waterJets;

    // 水帘数据结构
    struct CurtainInfo {
        QVector3D position;
        float width;
        float height;
        GLuint textureId = 0;
        QVector<QImage> images;
        int currentIndex = 0;
        float offset = 0.0f;          // 纹理偏移
        float offsetSpeed = 0.0f;     // 偏移速度 (1/秒)
    } m_curtain;

    // Uniform 位置
    int m_uniformMVP = 0;
    int m_uniformTime = 0;
    int m_curtainUniformMVP = 0;
    int m_curtainUniformTex = 0;
    int m_curtainUniformOffset = 0;

    // 常量
    static const int MAX_PARTICLES = 10000;
    static const float GROUND_Y;

    // 配置参数
    int m_fountainCount;
    float m_fountainSpacing;
    float m_startX;
    float m_fountainHeight;
    float m_waterJetLength;
    float m_waterJetTopWidth;
    float m_waterJetBottomWidth;
    float m_spawnRate;
    float m_particleMinSize;
    float m_particleMaxSize;
    float m_particleMinLife;
    float m_particleMaxLife;
    float m_particleSpeedX;
    float m_particleSpeedYMin;
    float m_particleSpeedYMax;
    float m_particleSpeedZ;
    float m_poolWidth;
    float m_poolDepth;
    float m_waterAlpha;
    QVector3D m_waterColor;
    float m_windStrength;
    float m_windDirection;
    float m_arcRadius;
    float m_arcAngle;

    // 音乐同步
    bool m_musicSyncEnabled;
    float m_musicSensitivity;

    // 运行时
    float m_spawnTimer;
    float m_lastTime;
    int m_timerId;
    bool m_initialized;

    QElapsedTimer m_elapsedTimer;
};