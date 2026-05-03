#include "chart.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QFileInfo>
#include <algorithm>

Chart::Chart() {}

void Chart::addNote(const Note& note) {
    notes.append(note);
}

void Chart::removeNote(int index) {
    if (index >= 0 && index < notes.size())
        notes.removeAt(index);
}

void Chart::clear() {
    notes.clear();
    musicFilePath.clear();
}

void Chart::sortNotes() {
    std::sort(notes.begin(), notes.end(), [](const Note& a, const Note& b) {
        return a.startTimeMs < b.startTimeMs;
    });
}

bool Chart::saveToFile(const QString& filePath) const {
    QJsonObject root;
    root["musicFile"] = musicFilePath;
    root["version"] = 1;

    QJsonArray noteArray;
    for (const auto& note : notes) {
        QJsonObject obj;
        obj["track"] = note.track;
        obj["color"] = (note.color == NoteColor::Blue) ? "blue" : "yellow";
        obj["startTime"] = static_cast<qint64>(note.startTimeMs);
        obj["duration"] = static_cast<qint64>(note.durationMs);
        noteArray.append(obj);
    }
    root["notes"] = noteArray;

    QJsonDocument doc(root);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(doc.toJson());
    return true;
}

bool Chart::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject())
        return false;

    QJsonObject root = doc.object();
    musicFilePath = root["musicFile"].toString();
    QJsonArray noteArray = root["notes"].toArray();

    notes.clear();
    for (const auto& val : noteArray) {
        QJsonObject obj = val.toObject();
        Note n;
        n.track = obj["track"].toInt(0);
        QString col = obj["color"].toString("blue");
        n.color = (col == "yellow") ? NoteColor::Yellow : NoteColor::Blue;
        n.startTimeMs = static_cast<qint64>(obj["startTime"].toVariant().toLongLong());
        n.durationMs = static_cast<qint64>(obj["duration"].toVariant().toLongLong());
        n.hit = false;
        n.missed = false;
        notes.append(n);
    }
    return true;
}

QString Chart::suggestedChartPath() const {
    if (musicFilePath.isEmpty()) return QString();
    QFileInfo info(musicFilePath);
    return info.absolutePath() + "/" + info.completeBaseName() + ".json";
}
