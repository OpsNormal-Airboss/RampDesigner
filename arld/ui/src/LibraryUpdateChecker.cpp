#include <arld/ui/LibraryUpdateChecker.h>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>

namespace arld::ui {

LibraryUpdateChecker::LibraryUpdateChecker(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this)) {
}

void LibraryUpdateChecker::checkForUpdates(const QString& manifestUrl) {
    // HTTPS-only security check
    const QUrl url(manifestUrl);
    if (url.scheme().toLower() != QStringLiteral("https")) {
        emit checkFailed(tr("Library update check requires an HTTPS URL."));
        return;
    }

    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    m_manifestReply = m_nam->get(req);
    connect(m_manifestReply, &QNetworkReply::finished,
            this, &LibraryUpdateChecker::onManifestReply);
}

void LibraryUpdateChecker::onManifestReply() {
    if (!m_manifestReply) return;
    m_manifestReply->deleteLater();

    if (m_manifestReply->error() != QNetworkReply::NoError) {
        emit checkFailed(tr("Network error: %1").arg(m_manifestReply->errorString()));
        m_manifestReply = nullptr;
        return;
    }

    const QByteArray data = m_manifestReply->readAll();
    m_manifestReply = nullptr;

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit checkFailed(tr("Invalid manifest JSON."));
        return;
    }

    // Load the embedded local manifest
    QFile localManifestFile(QStringLiteral(":/library/library_manifest.json"));
    if (!localManifestFile.open(QIODevice::ReadOnly)) {
        emit checkFailed(tr("Cannot open local library manifest."));
        return;
    }
    const QJsonDocument localDoc = QJsonDocument::fromJson(localManifestFile.readAll());
    localManifestFile.close();

    // Build a set of local entry IDs and their SHA-256 hashes (if available)
    QHash<QString, QByteArray> localHashes;
    if (localDoc.isObject()) {
        const QJsonArray entries = localDoc.object().value(QStringLiteral("entries")).toArray();
        for (const QJsonValue& val : entries) {
            const QString id = val.toObject().value(QStringLiteral("id")).toString();
            if (id.isEmpty()) continue;
            // Compute SHA-256 of the embedded resource content
            QFile entryFile(QStringLiteral(":/library/%1.json").arg(id));
            if (entryFile.open(QIODevice::ReadOnly)) {
                const QByteArray hash = QCryptographicHash::hash(
                    entryFile.readAll(), QCryptographicHash::Sha256).toHex();
                localHashes[id] = hash;
                entryFile.close();
            }
        }
    }

    // Compare with the remote manifest
    QStringList updatedIds;
    const QJsonArray remoteEntries = doc.object()
        .value(QStringLiteral("entries")).toArray();

    for (const QJsonValue& val : remoteEntries) {
        const QJsonObject obj = val.toObject();
        const QString id = obj.value(QStringLiteral("id")).toString();
        if (id.isEmpty()) continue;

        const QString remoteHash = obj.value(QStringLiteral("sha256")).toString();
        if (remoteHash.isEmpty()) {
            // No hash in remote manifest: treat all entries as needing update check
            updatedIds << id;
        } else {
            // Compare hashes; different or not present locally → update available
            if (!localHashes.contains(id) ||
                localHashes[id] != remoteHash.toLatin1()) {
                updatedIds << id;
            }
        }
    }

    if (updatedIds.isEmpty()) {
        emit upToDate();
    } else {
        emit updatesAvailable(updatedIds);
    }
}

void LibraryUpdateChecker::downloadUpdates(const QStringList& ids, const QString& baseUrl) {
    if (ids.isEmpty()) return;

    // HTTPS-only
    const QUrl base(baseUrl);
    if (base.scheme().toLower() != QStringLiteral("https")) {
        emit checkFailed(tr("Download requires an HTTPS base URL."));
        return;
    }

    m_pendingIds    = ids;
    m_baseUrl       = baseUrl;
    m_total         = ids.size();
    m_completed     = 0;
    m_downloadedIds.clear();

    // Kick off the first download
    onEntryReply(); // processes queue via chain
    // Actually start the first request immediately:
    m_completed = 0; // reset since onEntryReply was called prematurely
    for (const QString& id : m_pendingIds) {
        const QUrl entryUrl(baseUrl + QStringLiteral("/") + id + QStringLiteral(".json"));
        QNetworkRequest req(entryUrl);
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply* reply = m_nam->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, id] {
            reply->deleteLater();
            ++m_completed;
            emit downloadProgress(m_completed, m_total);

            if (reply->error() != QNetworkReply::NoError) {
                // Non-fatal: skip this entry
                return;
            }

            const QByteArray entryData = reply->readAll();

            // Basic JSON validation — don't overwrite with garbage
            const QJsonDocument doc = QJsonDocument::fromJson(entryData);
            if (!doc.isObject()) return;

            // Write to user data directory
            const QString dataDir = QStandardPaths::writableLocation(
                QStandardPaths::AppDataLocation) + QStringLiteral("/library");
            QDir().mkpath(dataDir);
            QFile outFile(dataDir + QStringLiteral("/") + id + QStringLiteral(".json"));
            if (outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                outFile.write(entryData);
                m_downloadedIds.append(id);
            }

            if (m_completed == m_total)
                emit downloadCompleted(m_downloadedIds);
        });
    }
    m_pendingIds.clear();
}

void LibraryUpdateChecker::onEntryReply() {
    // Stub — actual work done inline in downloadUpdates lambda above
}

void LibraryUpdateChecker::cancelUpdate() {
    m_pendingIds.clear();
    if (m_manifestReply) {
        m_manifestReply->abort();
        m_manifestReply = nullptr;
    }
}

} // namespace arld::ui
