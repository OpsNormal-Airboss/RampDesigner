#pragma once
#include <QFutureWatcher>
#include <QGraphicsItem>
#include <QImage>
#include <QObject>
#include <QRectF>
#include <QString>

namespace arld::ui {

/// QGraphicsItem that loads a raster image asynchronously and renders it
/// as a satellite/aerial underlay beneath the ramp layout.
/// zValue = -2.0 (behind everything else in the scene).
class SatelliteUnderlayItem : public QObject, public QGraphicsItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    explicit SatelliteUnderlayItem(QGraphicsItem* parent = nullptr);

    /// Asynchronously load an image from @p path.
    /// Emits imageLoaded() when complete.
    void loadImage(const QString& path);

    /// Set the render opacity (0.0 = invisible, 1.0 = fully opaque).
    void setOpacity(float opacity);

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void   paint(QPainter* painter,
                 const QStyleOptionGraphicsItem* option,
                 QWidget* widget) override;

signals:
    void imageLoaded();

private slots:
    void onImageReady();

private:
    QImage  m_image;
    float   m_opacity = 0.8f;
    bool    m_loading = false;
    QRectF  m_rect;   // scene-space bounding rect (pixels = feet for now)
    QFutureWatcher<QImage> m_watcher;
};

} // namespace arld::ui
