#ifndef EDITORWIDGET_H
#define EDITORWIDGET_H

#include "chart.h"
#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>

class QPushButton;
class QSlider;
class QLabel;
class QComboBox;
class QSpinBox;

class EditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit EditorWidget(QWidget *parent = nullptr);
    ~EditorWidget();

    void loadMusic(const QString& path);
    void stopEditor();

signals:
    void returnToMenu();
    void musicLoaded(const QString& path);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onPlayPause();
    void onStop();
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onSliderMoved(int value);
    void onSaveChart();
    void onLoadChart();
    void onTick();

private:
    void setupUI();
    int trackAt(int x) const;
    int trackX(int track) const;
    int trackWidth() const;
    int timeToY(qint64 timeMs) const;
    qint64 yToTime(int y) const;
    void addNoteAt(int track, qint64 timeMs);

    QMediaPlayer* m_player = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    QTimer* m_timer = nullptr;

    Chart m_chart;
    bool m_isPlaying = false;

    // Controls
    QPushButton* m_btnPlay = nullptr;
    QPushButton* m_btnStop = nullptr;
    QPushButton* m_btnSave = nullptr;
    QPushButton* m_btnLoad = nullptr;
    QPushButton* m_btnAuto = nullptr;
    QPushButton* m_btnBack = nullptr;
    QSlider* m_slider = nullptr;
    QLabel* m_lblTime = nullptr;
    QComboBox* m_cmbColor = nullptr;
    QSpinBox* m_spinDuration = nullptr;
    QLabel* m_lblStatus = nullptr;

    // Visual settings
    static constexpr qreal PIXELS_PER_MS = 0.45;
    static constexpr int TRACK_COUNT = 5;
};

#endif // EDITORWIDGET_H
