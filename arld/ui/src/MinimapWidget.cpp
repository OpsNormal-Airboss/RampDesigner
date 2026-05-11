#include <arld/ui/MinimapWidget.h>
#include <arld/ui/RampScene.h>
#include <arld/ui/RampView.h>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>

namespace arld::ui {

MinimapWidget::MinimapWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(200, 150);
    setToolTip(tr("Minimap — click to pan"));
}

void MinimapWidget::setScene(arld::ui::RampScene* scene) {
    if (m_scene) disconnect(m_scene, nullptr, this, nullptr);
    m_scene = scene;
    if (m_scene) {
        connect(m_scene, &QGraphicsScene::changed, this, [this](const QList<QRectF>&) {
            update();
        });
    }
    update();
}

void MinimapWidget::setView(arld::ui::RampView* view) {
    if (m_view) disconnect(m_view, nullptr, this, nullptr);
    m_view = view;
    if (m_view) {
        // Repaint when the viewport changes (scroll/zoom)
        connect(m_view, &RampView::scaleChanged, this, [this](double) { update(); });
    }
    update();
}

void MinimapWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Dark background
    p.fillRect(rect(), QColor(30, 30, 30));

    if (!m_scene) return;

    const QRectF sceneRect = m_scene->itemsBoundingRect();
    if (sceneRect.isNull() || sceneRect.isEmpty()) {
        p.setPen(QColor(120, 120, 120));
        p.drawText(rect(), Qt::AlignCenter, tr("(empty)"));
        return;
    }

    // Compute scale-to-fit transform: scene → minimap widget
    const double scaleX = (width()  - 4) / sceneRect.width();
    const double scaleY = (height() - 4) / sceneRect.height();
    const double scale  = std::min(scaleX, scaleY);

    const double offX = (width()  - sceneRect.width()  * scale) / 2.0;
    const double offY = (height() - sceneRect.height() * scale) / 2.0;

    auto toWidget = [&](QPointF sp) -> QPointF {
        return QPointF(offX + (sp.x() - sceneRect.left()) * scale,
                       offY + (sp.y() - sceneRect.top())  * scale);
    };

    // Draw aircraft as small filled rectangles
    const QPen aircraftPen(QColor(100, 180, 100), 0);
    p.setPen(aircraftPen);
    p.setBrush(QColor(60, 140, 60, 180));
    for (const QGraphicsItem* item : m_scene->items()) {
        if (item->zValue() < -0.1) continue; // skip satellite/grid underlay
        const QRectF br = item->mapToScene(item->boundingRect()).boundingRect();
        if (br.isNull() || br.isEmpty()) continue;
        // Map bounding rect to minimap coords
        const QRectF wr(toWidget(br.topLeft()), toWidget(br.bottomRight()));
        if (wr.width() < 0.5 || wr.height() < 0.5)
            p.drawPoint(wr.center());
        else
            p.drawRect(wr);
    }

    // Draw viewport rectangle (blue semi-transparent)
    if (m_view) {
        const QRectF viewportScene = m_view->mapToScene(m_view->viewport()->rect()).boundingRect();
        const QRectF vwr(toWidget(viewportScene.topLeft()),
                         toWidget(viewportScene.bottomRight()));
        p.setPen(QPen(QColor(80, 160, 255), 1.5));
        p.setBrush(QColor(80, 160, 255, 40));
        p.drawRect(vwr);
    }
}

void MinimapWidget::mousePressEvent(QMouseEvent* event) {
    if (!m_scene || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    const QRectF sceneRect = m_scene->itemsBoundingRect();
    if (sceneRect.isNull() || sceneRect.isEmpty()) return;

    const double scaleX = (width()  - 4) / sceneRect.width();
    const double scaleY = (height() - 4) / sceneRect.height();
    const double scale  = std::min(scaleX, scaleY);

    const double offX = (width()  - sceneRect.width()  * scale) / 2.0;
    const double offY = (height() - sceneRect.height() * scale) / 2.0;

    const QPointF wp = event->position();
    const double sx = sceneRect.left() + (wp.x() - offX) / scale;
    const double sy = sceneRect.top()  + (wp.y() - offY) / scale;

    emit minimapClicked(QPointF(sx, sy));
    event->accept();
}

} // namespace arld::ui
