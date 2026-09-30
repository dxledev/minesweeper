#pragma once

#include <QPixmap>
#include <QWidget>

namespace minesweeper {

class ModalBackdrop : public QWidget {
public:
    explicit ModalBackdrop(QWidget *parent);
    void capture(const QPixmap &snapshot);
    void clearSnapshot();
    void setTint(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap snapshot_;
    QColor tint_;
};

}
