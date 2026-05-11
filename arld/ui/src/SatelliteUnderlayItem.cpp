#include <arld/ui/SatelliteUnderlayItem.h>
#include <QPainter>
#include <QtConcurrent/QtConcurrent>

namespace arld::ui {

SatelliteUnderlayItem::SatelliteUnderlayItem(QGraphicsItem* parent)
    : QObject(nullptr)
    , QGraphicsItem(parent)
{
    setZValue(-2.0);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);

    connect(&m_watcher, &QFutureWatcher<QImage>::finished,
            this, &SatelliteUnderlayItem::onImageReady);
}

void SatelliteUnderlayItem::loadImage(const QString& path) {
    m_loading = true;
    m_image   = QImage{};
    m_rect    = QRectF{};

    auto future = QtConcurrent::run([path]() -> QImage {
        return QImage(path);
    });
    m_watcher.setFuture(future);
}

void SatelliteUnderlayItem::setOpacity(float opacity) {
    m_opacity = std::clamp(opacity, 0.0f, 1.0f);
    update();
}

QRectF SatelliteUnderlayItem::boundingRect() const {
    return m_rect;
}

void SatelliteUnderlayItem::paint(QPainter* painter,
                                   const QStyleOptionGraphicsItem* /*option*/,
                                   QWidget* /*widget*/) {
    if (m_loading && m_image.isNull()) {
        painter->setPen(Qt::darkGray);
        painter->drawText(QRectF(-200, -20, 400, 40),
                          Qt::AlignCenter,
                          QStringLiteral("Loading satellite image..."));
        return;
    }

    if (!m_image.isNull()) {
        painter->setOpacity(static_cast<double>(m_opacity));
        painter->drawImage(m_rect, m_image);
        painter->setOpacity(1.0);
    }
}

void SatelliteUnderlayItem::onImageReady() {
    QImage img = m_watcher.result();
    m_loading  = false;

    if (!img.isNull()) {
        prepareGeometryChange();
        m_image = img;
        // 1 pixel = 1 foot (user can reposition/scale manually for now)
        m_rect  = QRectF(0.0, 0.0,
                         static_cast<double>(img.width()),
                         static_cast<double>(img.height()));
    }

    update();
    emit imageLoaded();
}

} // namespace arld::ui
