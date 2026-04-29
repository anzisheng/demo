#include "ConfigManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

ConfigManager& ConfigManager::getInstance()
{
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open config file:" << filePath << ", using default values";
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qDebug() << "Invalid JSON format, using default values";
        return false;
    }

    QJsonObject obj = doc.object();

    // 基础
    if (obj.contains("fountainCount")) m_fountainCount = obj["fountainCount"].toInt();
    if (obj.contains("fountainSpacing")) m_fountainSpacing = obj["fountainSpacing"].toDouble();
    if (obj.contains("startX")) m_startX = obj["startX"].toDouble();
    if (obj.contains("fountainHeight")) m_fountainHeight = obj["fountainHeight"].toDouble();

    // 水柱
    if (obj.contains("waterJetLength")) m_waterJetLength = obj["waterJetLength"].toDouble();
    if (obj.contains("waterJetTopWidth")) m_waterJetTopWidth = obj["waterJetTopWidth"].toDouble();
    if (obj.contains("waterJetBottomWidth")) m_waterJetBottomWidth = obj["waterJetBottomWidth"].toDouble();

    // 物理
    if (obj.contains("gravity")) m_gravity = obj["gravity"].toDouble();
    if (obj.contains("spawnRate")) m_spawnRate = obj["spawnRate"].toDouble();

    // 粒子
    if (obj.contains("particleMinSize")) m_particleMinSize = obj["particleMinSize"].toDouble();
    if (obj.contains("particleMaxSize")) m_particleMaxSize = obj["particleMaxSize"].toDouble();
    if (obj.contains("particleMinLife")) m_particleMinLife = obj["particleMinLife"].toDouble();
    if (obj.contains("particleMaxLife")) m_particleMaxLife = obj["particleMaxLife"].toDouble();
    if (obj.contains("particleSpeedX")) m_particleSpeedX = obj["particleSpeedX"].toDouble();
    if (obj.contains("particleSpeedYMin")) m_particleSpeedYMin = obj["particleSpeedYMin"].toDouble();
    if (obj.contains("particleSpeedYMax")) m_particleSpeedYMax = obj["particleSpeedYMax"].toDouble();
    if (obj.contains("particleSpeedZ")) m_particleSpeedZ = obj["particleSpeedZ"].toDouble();

    // 水池
    if (obj.contains("poolWidth")) m_poolWidth = obj["poolWidth"].toDouble();
    if (obj.contains("poolDepth")) m_poolDepth = obj["poolDepth"].toDouble();

    // 视觉
    if (obj.contains("waterColor")) {
        QJsonArray colorArr = obj["waterColor"].toArray();
        if (colorArr.size() >= 3) {
            m_waterColor = QVector3D(colorArr[0].toDouble(), colorArr[1].toDouble(), colorArr[2].toDouble());
        }
    }
    if (obj.contains("waterAlpha")) m_waterAlpha = obj["waterAlpha"].toDouble();

    // 风
    if (obj.contains("windStrength")) m_windStrength = obj["windStrength"].toDouble();
    if (obj.contains("windDirection")) m_windDirection = obj["windDirection"].toDouble();

    // 弧形
    if (obj.contains("arcRadius")) m_arcRadius = obj["arcRadius"].toDouble();
    if (obj.contains("arcAngle")) m_arcAngle = obj["arcAngle"].toDouble();

    // 水帘
    if (obj.contains("curtainWidth")) m_curtainWidth = obj["curtainWidth"].toDouble();
    if (obj.contains("curtainHeight")) m_curtainHeight = obj["curtainHeight"].toDouble();
    if (obj.contains("curtainImagePath")) m_curtainImagePath = obj["curtainImagePath"].toString();
    if (obj.contains("curtainFallDuration")) m_curtainFallDuration = obj["curtainFallDuration"].toDouble();

    // 地面喷泉
    if (obj.contains("groundFountainCount")) m_groundFountainCount = obj["groundFountainCount"].toInt();
    if (obj.contains("groundFountainRadius")) m_groundFountainRadius = obj["groundFountainRadius"].toDouble();

    qDebug() << "Config loaded successfully, fountain count:" << m_fountainCount;
    return true;
}

void ConfigManager::saveConfig(const QString& filePath)
{
    QJsonObject obj;
    obj["fountainCount"] = m_fountainCount;
    obj["fountainSpacing"] = m_fountainSpacing;
    obj["startX"] = m_startX;
    obj["fountainHeight"] = m_fountainHeight;
    obj["waterJetLength"] = m_waterJetLength;
    obj["waterJetTopWidth"] = m_waterJetTopWidth;
    obj["waterJetBottomWidth"] = m_waterJetBottomWidth;
    obj["gravity"] = m_gravity;
    obj["spawnRate"] = m_spawnRate;
    obj["particleMinSize"] = m_particleMinSize;
    obj["particleMaxSize"] = m_particleMaxSize;
    obj["particleMinLife"] = m_particleMinLife;
    obj["particleMaxLife"] = m_particleMaxLife;
    obj["particleSpeedX"] = m_particleSpeedX;
    obj["particleSpeedYMin"] = m_particleSpeedYMin;
    obj["particleSpeedYMax"] = m_particleSpeedYMax;
    obj["particleSpeedZ"] = m_particleSpeedZ;
    obj["poolWidth"] = m_poolWidth;
    obj["poolDepth"] = m_poolDepth;
    QJsonArray colorArr;
    colorArr.append(m_waterColor.x());
    colorArr.append(m_waterColor.y());
    colorArr.append(m_waterColor.z());
    obj["waterColor"] = colorArr;
    obj["waterAlpha"] = m_waterAlpha;
    obj["windStrength"] = m_windStrength;
    obj["windDirection"] = m_windDirection;
    obj["arcRadius"] = m_arcRadius;
    obj["arcAngle"] = m_arcAngle;
    obj["curtainWidth"] = m_curtainWidth;
    obj["curtainHeight"] = m_curtainHeight;
    obj["curtainImagePath"] = m_curtainImagePath;
    obj["curtainFallDuration"] = m_curtainFallDuration;
    obj["groundFountainCount"] = m_groundFountainCount;
    obj["groundFountainRadius"] = m_groundFountainRadius;

    QJsonDocument doc(obj);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        qDebug() << "Config saved to:" << filePath;
    }
}