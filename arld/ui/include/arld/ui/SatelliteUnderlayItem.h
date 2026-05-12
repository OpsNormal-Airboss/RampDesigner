#pragma once
#include <QFutureWatcher>
#include <QGraphicsItem>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QRectF>
#include <QString>
#include <QTemporaryFile>
#include <memory>

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

    /// Fetch a tile from @p url (HTTPS only).
    /// If url does not start with "https://", emits loadingError("HTTPS required") and returns.
    /// On success, writes to a temp file and calls loadImage().
    void fetchTile(const QString& url);

    /// Set the render opacity (0.0 = invisible, 1.0 = fully opaque).
    void setOpacity(float opacity);

    /// Configure georeferencing so the image is scaled to real-world feet.
    /// @p latDeg     centre latitude of the tile (decimal degrees)
    /// @p zoomLevel  Web Mercator zoom level (0–22)
    /// @p highDpi    true for @2x tiles (512 px represents one 256-px tile step)
    /// Must be called before or after loadImage()/fetchTile(); takes effect on
    /// the next onImageReady() call (i.e., call before fetchTile).
    void setGeoreference(double latDeg, int zoomLevel, bool highDpi = false);

    /// Directly set the ground sample distance (feet per image pixel).
    /// Takes effect immediately if an image is already loaded.
    void setFeetPerPixel(double feetPerPixel);

    /// Return the current GSD in feet per pixel.
    double feetPerPixel() const { return m_feetPerPixel; }

    /// Reset the underlay to empty — clears image, rect, and aborts any in-progress fetch.
    /// Called by RampScene::clearScene() when starting a new project.
    void clear();

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void   paint(QPainter* painter,
                 const QStyleOptionGraphicsItem* option,
                 QWidget* widget) override;

signals:
    void imageLoaded();
    void loadingFinished(bool ok);
    void loadingError(const QString& message);

private slots:
    void onImageReady();
    void onNetworkReplyFinished();

private:
    QImage  m_image;
    float   m_opacity     = 0.8f;
    bool    m_loading     = false;
    double  m_feetPerPixel = 1.0;   // GSD: feet represented by one image pixel
    QRectF  m_rect;                 // scene-space bounding rect in feet
    QFutureWatcher<QImage>   m_watcher;
    QNetworkAccessManager    m_nam;
    QNetworkReply*           m_reply = nullptr;
    std::unique_ptr<QTemporaryFile> m_tempFile;
};

} // namespace arld::ui
