#include <arld/ui/SatelliteUnderlayItem.h>
#include <QDir>
#include <QNetworkRequest>
#include <QPainter>
#include <QUrl>
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
    emit loadingFinished(!img.isNull());
}

void SatelliteUnderlayItem::fetchTile(const QString& url) {
    // HTTPS only — reject immediately without any network call
    if (!url.startsWith(QStringLiteral("https://"))) {
        emit loadingError(tr("HTTPS required"));
        return;
    }

    // Cancel any pending reply
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    m_loading = true;
    update();

    QUrl parsedUrl(url);
    QNetworkRequest req(parsedUrl);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("ARLD/2.0"));
    m_reply = m_nam.get(req);
    connect(m_reply, &QNetworkReply::finished,
            this, &SatelliteUnderlayItem::onNetworkReplyFinished);
}

void SatelliteUnderlayItem::onNetworkReplyFinished() {
    if (!m_reply) return;

    const bool ok = (m_reply->error() == QNetworkReply::NoError);
    if (ok) {
        const QByteArray data = m_reply->readAll();
        // Write to a temp file, then load from disk
        m_tempFile = std::make_unique<QTemporaryFile>();
        m_tempFile->setFileTemplate(QStringLiteral("%1/arld_tile_XXXXXX.jpg")
                                        .arg(QDir::tempPath()));
        if (m_tempFile->open()) {
            m_tempFile->write(data);
            m_tempFile->flush();
            loadImage(m_tempFile->fileName());
        } else {
            m_loading = false;
            emit loadingError(tr("Failed to write temporary tile file"));
            emit loadingFinished(false);
        }
    } else {
        m_loading = false;
        emit loadingError(m_reply->errorString());
        emit loadingFinished(false);
        update();
    }

    m_reply->deleteLater();
    m_reply = nullptr;
}

} // namespace arld::ui
