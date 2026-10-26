#pragma once

#include <QPixmap>
#include <QPointer>
#include <QWidget>

namespace minesweeper {

class ModalMotion;

class ModalBackdrop : public QWidget {
    Q_OBJECT
public:
    explicit ModalBackdrop(QWidget *parent);
    void capture(const QPixmap &snapshot);
    void clearSnapshot();
    void setTint(const QColor &color);
    // Fades the backdrop in and scales/fades the card in; the widget must already be shown.
    void present(QWidget *card);
    void dismiss();
    bool closing() const { return closing_; }

signals:
    void dismissed();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap frame_, snapshot_;
    QColor tint_;
    QPointer<QWidget> card_;
    QPointer<ModalMotion> motion_;
    qreal progress_ = 1;
    bool closing_ = false;
};

}
