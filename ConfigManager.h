#pragma once
#include <QString>
#include <QJsonObject>
#include <QVector3D>

class ConfigManager
{
public:
    static ConfigManager& getInstance();
    bool loadConfig(const QString& filePath = "FountainConfig.json");
    void saveConfig(const QString& filePath = "FountainConfig.json");

    // 水阀阵列参数
    int getWaterValveGridWidth() const { return m_waterValveGridWidth; }
    int getWaterValveGridHeight() const { return m_waterValveGridHeight; }
    float getWaterValveSpacing() const { return m_waterValveSpacing; }
    QString getWaterValveImagePath() const { return m_waterValveImagePath; }
    float getWaterValveBaseHeight() const { return m_waterValveBaseHeight; }
    float getWaterValveMaxLength() const { return m_waterValveMaxLength; }

    // 水帘参数
    float getCurtainWidth() const { return m_curtainWidth; }
    float getCurtainHeight() const { return m_curtainHeight; }
    QString getCurtainImagePath() const { return m_curtainImagePath; }
    float getCurtainFallDuration() const { return m_curtainFallDuration; }

    // 粒子系统参数
    float getSpawnRate() const { return m_spawnRate; }
    float getParticleMinSize() const { return m_particleMinSize; }
    float getParticleMaxSize() const { return m_particleMaxSize; }
    float getParticleMinLife() const { return m_particleMinLife; }
    float getParticleMaxLife() const { return m_particleMaxLife; }
    float getParticleSpeedX() const { return m_particleSpeedX; }
    float getParticleSpeedYMin() const { return m_particleSpeedYMin; }
    float getParticleSpeedYMax() const { return m_particleSpeedYMax; }
    float getParticleSpeedZ() const { return m_particleSpeedZ; }
    float getGravity() const { return m_gravity; }

    // 水池
    float getPoolWidth() const { return m_poolWidth; }
    float getPoolDepth() const { return m_poolDepth; }
    QVector3D getWaterColor() const { return m_waterColor; }
    float getWaterAlpha() const { return m_waterAlpha; }

    // 风
    float getWindStrength() const { return m_windStrength; }
    float getWindDirection() const { return m_windDirection; }

private:
    ConfigManager() = default;

    // 水阀阵列
    int m_waterValveGridWidth = 40;
    int m_waterValveGridHeight = 25;
    float m_waterValveSpacing = 0.35f;
    QString m_waterValveImagePath = "./valve_image.bmp";
    float m_waterValveBaseHeight = -0.5f;
    float m_waterValveMaxLength = 1.5f;

    // 水帘
    float m_curtainWidth = 12.0f;
    float m_curtainHeight = 6.0f;
    QString m_curtainImagePath = "./resources/";
    float m_curtainFallDuration = 4.0f;

    // 粒子
    float m_spawnRate = 0.012f;
    float m_particleMinSize = 0.04f;
    float m_particleMaxSize = 0.07f;
    float m_particleMinLife = 0.8f;
    float m_particleMaxLife = 1.2f;
    float m_particleSpeedX = 1.2f;
    float m_particleSpeedYMin = -3.0f;
    float m_particleSpeedYMax = -1.5f;
    float m_particleSpeedZ = 1.2f;
    float m_gravity = -9.8f;

    // 水池
    float m_poolWidth = 18.0f;
    float m_poolDepth = 12.0f;
    QVector3D m_waterColor = QVector3D(0.2f, 0.65f, 0.95f);
    float m_waterAlpha = 0.65f;

    // 风
    float m_windStrength = 0.5f;
    float m_windDirection = 0.3f;
};