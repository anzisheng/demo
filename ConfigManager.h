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

    // 基础
    int getFountainCount() const { return m_fountainCount; }
    float getFountainSpacing() const { return m_fountainSpacing; }
    float getStartX() const { return m_startX; }
    float getFountainHeight() const { return m_fountainHeight; }

    // 水柱
    float getWaterJetLength() const { return m_waterJetLength; }
    float getWaterJetTopWidth() const { return m_waterJetTopWidth; }
    float getWaterJetBottomWidth() const { return m_waterJetBottomWidth; }

    // 物理
    float getGravity() const { return m_gravity; }
    float getSpawnRate() const { return m_spawnRate; }

    // 粒子
    float getParticleMinSize() const { return m_particleMinSize; }
    float getParticleMaxSize() const { return m_particleMaxSize; }
    float getParticleMinLife() const { return m_particleMinLife; }
    float getParticleMaxLife() const { return m_particleMaxLife; }
    float getParticleSpeedX() const { return m_particleSpeedX; }
    float getParticleSpeedYMin() const { return m_particleSpeedYMin; }
    float getParticleSpeedYMax() const { return m_particleSpeedYMax; }
    float getParticleSpeedZ() const { return m_particleSpeedZ; }

    // 水池
    float getPoolWidth() const { return m_poolWidth; }
    float getPoolDepth() const { return m_poolDepth; }

    // 视觉
    QVector3D getWaterColor() const { return m_waterColor; }
    float getWaterAlpha() const { return m_waterAlpha; }

    // 风
    float getWindStrength() const { return m_windStrength; }
    float getWindDirection() const { return m_windDirection; }

    // 弧形
    float getArcRadius() const { return m_arcRadius; }
    float getArcAngle() const { return m_arcAngle; }

    // 水帘 (注意：interval已被移除，改用 fallDuration)
    float getCurtainWidth() const { return m_curtainWidth; }
    float getCurtainHeight() const { return m_curtainHeight; }
    QString getCurtainImagePath() const { return m_curtainImagePath; }
    float getCurtainFallDuration() const { return m_curtainFallDuration; }

    // 地面喷泉
    int getGroundFountainCount() const { return m_groundFountainCount; }
    float getGroundFountainRadius() const { return m_groundFountainRadius; }

    void setFountainCount(int count) { m_fountainCount = count; }
    void setSpawnRate(float rate) { m_spawnRate = rate; }
    void setWindStrength(float strength) { m_windStrength = strength; }
    void setWindDirection(float direction) { m_windDirection = direction; }

private:
    ConfigManager() = default;

    int m_fountainCount = 50;
    float m_fountainSpacing = 0.35f;
    float m_startX = -8.5f;
    float m_fountainHeight = 5.5f;
    float m_waterJetLength = 2.2f;
    float m_waterJetTopWidth = 0.08f;
    float m_waterJetBottomWidth = 0.02f;
    float m_gravity = -9.8f;
    float m_spawnRate = 0.012f;
    float m_particleMinSize = 0.04f;
    float m_particleMaxSize = 0.07f;
    float m_particleMinLife = 0.8f;
    float m_particleMaxLife = 1.2f;
    float m_particleSpeedX = 1.2f;
    float m_particleSpeedYMin = -3.0f;
    float m_particleSpeedYMax = -1.5f;
    float m_particleSpeedZ = 1.2f;
    float m_poolWidth = 18.0f;
    float m_poolDepth = 12.0f;
    QVector3D m_waterColor = QVector3D(0.2f, 0.65f, 0.95f);
    float m_waterAlpha = 0.65f;
    float m_windStrength = 0.5f;
    float m_windDirection = 0.3f;
    float m_arcRadius = 8.0f;
    float m_arcAngle = 120.0f;
    float m_curtainWidth = 12.0f;
    float m_curtainHeight = 6.0f;
    QString m_curtainImagePath = "./resources/";
    float m_curtainFallDuration = 4.0f;
    int m_groundFountainCount = 30;
    float m_groundFountainRadius = 5.0f;
};