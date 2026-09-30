#pragma once

#include <QFrame>
#include <QPushButton>

namespace minesweeper {

class HowToPlayCard : public QFrame {
public:
    explicit HowToPlayCard(QWidget *parent = nullptr);
    QPushButton *closeButton() const { return close_; }
    QSize sizeHint() const override;
    int heightForWidth(int width) const override;

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QPushButton *close_ = nullptr;
};

}
