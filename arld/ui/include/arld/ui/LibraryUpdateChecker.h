#pragma once
#include <QNetworkAccessManager>
#include <QObject>
#include <QStringList>

namespace arld::ui {

/// Checks for updated aircraft library entries against a remote manifest.
/// Network requests are HTTPS-only and are only initiated when checkForUpdates()
/// or downloadUpdates() is called.
class LibraryUpdateChecker : public QObject {
    Q_OBJECT
public:
    explicit LibraryUpdateChecker(QObject* parent = nullptr);

    /// Check for updates against the remote manifest URL.
    /// No network request is made until this is called.
    /// Rejects non-HTTPS URLs via checkFailed().
    void checkForUpdates(const QString& manifestUrl);

signals:
    void updatesAvailable(QStringList updatedIds);
    void upToDate();
    void checkFailed(const QString& error);
    void downloadProgress(int current, int total);
    /// Emitted when all requested entry downloads have completed (some may have failed).
    void downloadCompleted(QStringList downloadedIds);

public slots:
    /// Download the given entry IDs from baseUrl/{id}.json into the user data directory.
    void downloadUpdates(const QStringList& ids, const QString& baseUrl);
    void cancelUpdate();

private slots:
    void onManifestReply();
    void onEntryReply();

private:
    QNetworkAccessManager* m_nam;
    QStringList            m_pendingIds;
    QStringList            m_downloadedIds;
    int                    m_total      = 0;
    int                    m_completed  = 0;
    QString                m_baseUrl;
    QNetworkReply*         m_manifestReply = nullptr;
};

} // namespace arld::ui
