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
    // 在 ConfigManager 类中添加
    bool getAutoScale() const { return m_autoScale; }
    void setAutoScale(bool scale) { m_autoScale = scale; }

    // 水阀参数
    int getFountainCount() const { return m_fountainCount; }
    float getFountainSpacing() const { return m_fountainSpacing; }
    float getStartX() const { return m_startX; }
    float getFountainHeight() const { return m_fountainHeight; }

    // 水柱参数
    float getWaterJetLength() const { return m_waterJetLength; }
    float getWaterJetTopWidth() const { return m_waterJetTopWidth; }
    float getWaterJetBottomWidth() const { return m_waterJetBottomWidth; }

    // 物理参数
    float getGravity() const { return m_gravity; }
    float getSpawnRate() const { return m_spawnRate; }

    // 粒子参数
    float getParticleMinSize() const { return m_particleMinSize; }
    float getParticleMaxSize() const { return m_particleMaxSize; }
    float getParticleMinLife() const { return m_particleMinLife; }
    float getParticleMaxLife() const { return m_particleMaxLife; }
    float getParticleSpeedX() const { return m_particleSpeedX; }
    float getParticleSpeedYMin() const { return m_particleSpeedYMin; }
    float getParticleSpeedYMax() const { return m_particleSpeedYMax; }
    float getParticleSpeedZ() const { return m_particleSpeedZ; }

    // 水池参数
    float getPoolWidth() const { return m_poolWidth; }
    float getPoolDepth() const { return m_poolDepth; }

    // 视觉参数
    QVector3D getWaterColor() const { return m_waterColor; }
    float getWaterAlpha() const { return m_waterAlpha; }

    // 风参数
    float getWindStrength() const { return m_windStrength; }
    float getWindDirection() const { return m_windDirection; }

    // 设置参数
    void setFountainCount(int count) { m_fountainCount = count; }
    void setSpawnRate(float rate) { m_spawnRate = rate; }
    void setWindStrength(float strength) { m_windStrength = strength; }
    void setWindDirection(float direction) { m_windDirection = direction; }
    float getArcRadius() const { return m_arcRadius; }
    float getArcAngle() const { return m_arcAngle; }
    void setArcRadius(float radius) { m_arcRadius = radius; }
    void setArcAngle(float angle) { m_arcAngle = angle; }

private:
    ConfigManager() = default;
    // 在 private 成员中添加
    bool m_autoScale = true;
    float m_arcRadius = 8.0f;
    float m_arcAngle = 120.0f;
    // 水阀参数
    int m_fountainCount = 50;
    float m_fountainSpacing = 0.35f;
    float m_startX = -8.5f;
    float m_fountainHeight = 5.5f;

    // 水柱参数
    float m_waterJetLength = 2.2f;
    float m_waterJetTopWidth = 0.08f;
    float m_waterJetBottomWidth = 0.02f;

    // 物理参数
    float m_gravity = -9.8f;
    float m_spawnRate = 0.012f;

    // 粒子参数
    float m_particleMinSize = 0.04f;
    float m_particleMaxSize = 0.07f;
    float m_particleMinLife = 0.8f;
    float m_particleMaxLife = 1.2f;
    float m_particleSpeedX = 1.2f;
    float m_particleSpeedYMin = -3.0f;
    float m_particleSpeedYMax = -1.5f;
    float m_particleSpeedZ = 1.2f;

    // 水池参数
    float m_poolWidth = 18.0f;
    float m_poolDepth = 12.0f;

    // 视觉参数
    QVector3D m_waterColor = QVector3D(0.2f, 0.65f, 0.95f);
    float m_waterAlpha = 0.65f;

    // 风参数
    float m_windStrength = 0.5f;
    float m_windDirection = 0.3f;
};