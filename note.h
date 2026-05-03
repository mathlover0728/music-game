#ifndef NOTE_H
#define NOTE_H

#include <QVector>

enum class NoteColor {
    Blue,   // Corresponds to track key (A/D/G/J/L)
    Yellow  // All yellow notes use Space key
};

struct Note {
    int track;           // 0-4 for the 5 tracks
    NoteColor color;     // Blue or Yellow
    qint64 startTimeMs;  // Start time in ms from song beginning
    qint64 durationMs; // Duration: 0 for short press, >0 for long press
    bool hit;            // Whether this note has been successfully hit
    bool missed;         // Whether this note was missed

    Note(int t = 0, NoteColor c = NoteColor::Blue, qint64 start = 0, qint64 dur = 0)
        : track(t), color(c), startTimeMs(start), durationMs(dur), hit(false), missed(false) {}

    bool isShortPress() const { return durationMs < 200; }
    bool isLongPress() const { return durationMs >= 200; }
};

#endif // NOTE_H
