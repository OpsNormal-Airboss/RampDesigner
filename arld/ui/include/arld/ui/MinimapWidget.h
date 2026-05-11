#pragma once
#include <QWidget>

namespace arld::ui {

class RampScene;
class RampView;

class MinimapWidget : public QWidget {
    Q_OBJECT
public:
    explicit MinimapWidget(QWidget* parent = nullptr);

    void setScene(arld::ui::RampScene* scene);
    void setView(arld::ui::RampView*   view);

signals:
    void minimapClicked(QPointF scenePos);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    arld::ui::RampScene* m_scene = nullptr;
    arld::ui::RampView*  m_view  = nullptr;
};

} // namespace arld::ui
