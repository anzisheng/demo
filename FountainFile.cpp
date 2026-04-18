#include "FountainFile.h"

bool FountainFile::saveToFile(const QString& filePath, const QVector<FountainData>& fountains)
{
    QJsonObject root;
    root["version"] = "1.0";
    root["fountainCount"] = fountains.size();
    root["fountains"] = fountainsToJson(fountains);

    QJsonDocument doc(root);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Cannot save file:" << filePath;
        return false;
    }

    file.write(doc.toJson());
    file.close();
    return true;
}

bool FountainFile::loadFromFile(const QString& filePath, QVector<FountainData>& fountains)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot load file:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qDebug() << "Invalid JSON format";
        return false;
    }

    QJsonObject root = doc.object();
    if (root.contains("fountains")) {
        fountains = jsonToFountains(root["fountains"].toArray());
        return true;
    }

    return false;
}

bool FountainFile::exportToJson(const QString& filePath, const QVector<FountainData>& fountains)
{
    return saveToFile(filePath, fountains);
}

bool FountainFile::importFromJson(const QString& filePath, QVector<FountainData>& fountains)
{
    return loadFromFile(filePath, fountains);
}

QJsonArray FountainFile::fountainsToJson(const QVector<FountainData>& fountains)
{
    QJsonArray array;
    for (const auto& f : fountains) {
        QJsonObject obj;
        obj["id"] = f.id;
        obj["x"] = f.position.x();
        obj["y"] = f.position.y();
        obj["z"] = f.position.z();
        obj["height"] = f.height;
        obj["waterFlow"] = f.waterFlow;
        obj["sprayAngle"] = f.sprayAngle;
        obj["enabled"] = f.enabled;
        array.append(obj);
    }
    return array;
}

QVector<FountainData> FountainFile::jsonToFountains(const QJsonArray& array)
{
    QVector<FountainData> fountains;
    for (const auto& item : array) {
        QJsonObject obj = item.toObject();
        FountainData f;
        f.id = obj["id"].toInt();
        f.position = QVector3D(
            obj["x"].toDouble(),
            obj["y"].toDouble(),
            obj["z"].toDouble()
        );
        f.height = obj["height"].toDouble();
        f.waterFlow = obj["waterFlow"].toDouble();
        f.sprayAngle = obj["sprayAngle"].toDouble();
        f.enabled = obj["enabled"].toBool();
        fountains.append(f);
    }
    return fountains;
}