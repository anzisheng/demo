#pragma once
#include <QVector>
#include <QString>
#include <QVector3D>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDebug>
#include "FountainData.h"

class FountainFile
{
public:
    static bool saveToFile(const QString& filePath, const QVector<FountainData>& fountains);
    static bool loadFromFile(const QString& filePath, QVector<FountainData>& fountains);
    static bool exportToJson(const QString& filePath, const QVector<FountainData>& fountains);
    static bool importFromJson(const QString& filePath, QVector<FountainData>& fountains);

private:
    static QJsonArray fountainsToJson(const QVector<FountainData>& fountains);
    static QVector<FountainData> jsonToFountains(const QJsonArray& array);
};