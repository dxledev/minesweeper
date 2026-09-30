#pragma once

#include "board.h"
#include "theme_watcher.h"
#include <QButtonGroup>
#include <QElapsedTimer>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QTimer>

class QHBoxLayout;
class QVBoxLayout;

namespace minesweeper {

class ModalBackdrop;

class Window : public QMainWindow {
    Q_OBJECT
public:
    Window(QString configDirectory, Theme theme, Difficulty difficulty = Difficulty::Easy);
    Board *board() const { return board_; }
    const Game &game() const { return game_; }
    qint64 elapsedMilliseconds() const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    Game game_;
    ThemeWatcher *themeWatcher_;
    Board *board_ = nullptr;
    QLabel *mines_ = nullptr;
    QLabel *time_ = nullptr;
    QLabel *progress_ = nullptr;
    QLabel *status_ = nullptr;
    QLabel *subtitle_ = nullptr;
    QLabel *themeError_ = nullptr;
    QPushButton *pause_ = nullptr;
    QPushButton *flagMode_ = nullptr;
    QButtonGroup *difficulties_ = nullptr;
    QWidget *content_ = nullptr;
    ModalBackdrop *modal_ = nullptr;
    QPushButton *keepPlaying_ = nullptr;
    QPushButton *confirmNew_ = nullptr;
    QLabel *modalText_ = nullptr;
    Difficulty pendingDifficulty_ = Difficulty::Easy;
    QTimer ticker_;
    QTimer snapshotRefresh_;
    QElapsedTimer clock_;
    qint64 accumulatedMilliseconds_ = 0;
    bool paused_ = false;
    bool modalPaused_ = false;
    bool automaticPause_ = false;
    void buildInterface();
    QHBoxLayout *buildHeader();
    QHBoxLayout *buildDifficultySelector();
    QHBoxLayout *buildMetrics();
    QWidget *buildBoardCard();
    QVBoxLayout *buildFooter();
    void buildModal();
    void buildShortcuts();
    void applyTheme();
    void refresh();
    void moved();
    void requestGame(Difficulty difficulty);
    void startGame(Difficulty difficulty);
    void dismissModal();
    void captureModalBackground();
    void setPaused(bool paused);
    void stopClock();
};

}
