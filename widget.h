#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QStackedWidget>

class GameWidget;
class EditorWidget;
class QPushButton;
class QLabel;

class Widget : public QWidget {
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onImportMusic();
    void onOpenEditor();
    void onPlayGame();
    void onMinimize();
    void onClose();
    void onReturnToMenu();
    void onMusicLoaded(const QString& path);

private:
    void setupUI();
    void setupTitleBar();
    void setupMenu();

    // Window dragging
    bool m_dragging = false;
    QPoint m_dragStartPos;

    // Title bar
    QWidget* m_titleBar = nullptr;
    QPushButton* m_btnMinimize = nullptr;
    QPushButton* m_btnClose = nullptr;
    QLabel* m_titleLabel = nullptr;

    // Screens
    QStackedWidget* m_stack = nullptr;
    QWidget* m_menuScreen = nullptr;
    GameWidget* m_gameScreen = nullptr;
    EditorWidget* m_editorScreen = nullptr;

    // Current imported music
    QString m_musicPath;
};

#endif // WIDGET_H
