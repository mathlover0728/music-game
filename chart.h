#ifndef CHART_H
#define CHART_H

#include "note.h"
#include <QString>
#include <QVector>

class Chart {
public:
    Chart();

    QString musicFilePath;
    QVector<Note> notes;

    void addNote(const Note& note);
    void removeNote(int index);
    void clear();
    void sortNotes();

    bool saveToFile(const QString& filePath) const;
    bool loadFromFile(const QString& filePath);

    QString suggestedChartPath() const;
};

#endif // CHART_H
