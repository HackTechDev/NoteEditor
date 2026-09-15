#include "Session.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringConverter>
#include <QTextStream>
#include <algorithm>

namespace Session {

QString configDir()
{
    return QDir::homePath() + "/.noteeditor";
}

QString draftsDir()
{
    return configDir() + "/drafts";
}

QString docsDir()
{
    return configDir() + "/docs";
}

static QString sessionFile()
{
    return configDir() + "/session.json";
}

static QString indexFile()
{
    return configDir() + "/index.json";
}

static QJsonValue toJsonOrNull(const QString &s)
{
    return s.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(s);
}

static QString fromJsonOrEmpty(const QJsonValue &v)
{
    return v.isString() ? v.toString() : QString();
}

static QJsonObject loadJsonObject(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QJsonObject();
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    return doc.isObject() ? doc.object() : QJsonObject();
}

static bool writeJsonObject(const QString &path, const QJsonObject &obj)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    return true;
}

static bool writeTextFile(const QString &path, const QString &content)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    return true;
}

static QJsonObject metaToJson(const QString &filePath, const QString &defaultName, bool modified)
{
    QJsonObject o;
    o["file_path"] = toJsonOrNull(filePath);
    o["default_name"] = toJsonOrNull(defaultName);
    o["modified"] = modified;
    return o;
}

void saveSession(const QVector<TabSnapshot> &tabs, const QString &activeId)
{
    QDir().mkpath(draftsDir());

    QJsonObject index = loadJsonObject(indexFile());
    QJsonArray entries;

    for (const TabSnapshot &info : tabs) {
        writeTextFile(draftsDir() + "/" + info.id + ".txt", info.content);

        QJsonObject entry = metaToJson(info.filePath, info.defaultName, info.modified);
        entry["id"] = info.id;
        entries.append(entry);

        index[info.id] = metaToJson(info.filePath, info.defaultName, info.modified);
    }

    QJsonObject sessionObj;
    sessionObj["active_id"] = toJsonOrNull(activeId);
    sessionObj["tabs"] = entries;
    writeJsonObject(sessionFile(), sessionObj);

    writeJsonObject(indexFile(), index);
}

void saveDraft(const TabSnapshot &info)
{
    QDir().mkpath(draftsDir());
    writeTextFile(draftsDir() + "/" + info.id + ".txt", info.content);

    QJsonObject index = loadJsonObject(indexFile());
    index[info.id] = metaToJson(info.filePath, info.defaultName, info.modified);
    writeJsonObject(indexFile(), index);
}

QVector<TabSnapshot> loadSession(QString *activeId)
{
    if (activeId)
        activeId->clear();

    QFile f(sessionFile());
    if (!f.open(QIODevice::ReadOnly))
        return {};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return {};
    const QJsonObject obj = doc.object();

    if (activeId)
        *activeId = fromJsonOrEmpty(obj.value("active_id"));

    QVector<TabSnapshot> result;
    for (const QJsonValue &v : obj.value("tabs").toArray()) {
        const QJsonObject entry = v.toObject();
        const QString id = entry.value("id").toString();

        QFile draftFile(draftsDir() + "/" + id + ".txt");
        if (!draftFile.open(QIODevice::ReadOnly))
            continue;
        const QString content = QString::fromUtf8(draftFile.readAll());

        TabSnapshot snap;
        snap.id = id;
        snap.filePath = fromJsonOrEmpty(entry.value("file_path"));
        snap.defaultName = fromJsonOrEmpty(entry.value("default_name"));
        snap.modified = entry.value("modified").toBool(false);
        snap.content = content;
        result.append(snap);
    }
    return result;
}

QVector<DraftEntry> listDrafts()
{
    QDir dir(draftsDir());
    if (!dir.exists())
        return {};

    const QJsonObject index = loadJsonObject(indexFile());

    QHash<QString, QJsonObject> sessionMeta;
    {
        QFile f(sessionFile());
        if (f.open(QIODevice::ReadOnly)) {
            const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            if (doc.isObject()) {
                for (const QJsonValue &v : doc.object().value("tabs").toArray()) {
                    const QJsonObject entry = v.toObject();
                    sessionMeta[entry.value("id").toString()] = entry;
                }
            }
        }
    }

    QVector<DraftEntry> items;
    const QStringList files = dir.entryList(QStringList() << "*.txt", QDir::Files);
    for (const QString &name : files) {
        const QString id = name.left(name.length() - 4);
        const QFileInfo fi(dir.filePath(name));

        QJsonObject meta = index.value(id).toObject();
        if (meta.isEmpty() && sessionMeta.contains(id))
            meta = sessionMeta.value(id);

        DraftEntry entry;
        entry.id = id;
        entry.filePath = fromJsonOrEmpty(meta.value("file_path"));
        entry.defaultName = fromJsonOrEmpty(meta.value("default_name"));
        entry.modified = meta.contains("modified") ? meta.value("modified").toBool(true) : true;
        entry.mtimeMs = fi.lastModified().toMSecsSinceEpoch();
        items.append(entry);
    }

    std::sort(items.begin(), items.end(), [](const DraftEntry &a, const DraftEntry &b) {
        return a.mtimeMs > b.mtimeMs;
    });
    return items;
}

QString readDraft(const QString &draftId)
{
    QFile f(draftsDir() + "/" + draftId + ".txt");
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromUtf8(f.readAll());
}

void deleteDraft(const QString &draftId)
{
    QFile::remove(draftsDir() + "/" + draftId + ".txt");

    QJsonObject index = loadJsonObject(indexFile());
    if (index.contains(draftId)) {
        index.remove(draftId);
        writeJsonObject(indexFile(), index);
    }
}

} // namespace Session
