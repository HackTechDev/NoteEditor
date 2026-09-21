#include "Session.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QStringConverter>
#include <QTextStream>
#include <algorithm>

namespace Session {

static const int kMaxVersions = 10;

QString configDir()
{
    return QDir::homePath() + "/.noteeditor";
}

static QString historyDir()
{
    return configDir() + "/history";
}

static void saveHistories(const QVector<TabSnapshot> &tabs);

QString draftsDir()
{
    return configDir() + "/drafts";
}

QString docsDir()
{
    return configDir() + "/docs";
}

QString trashDir()
{
    return configDir() + "/trash";
}

QString versionsDir()
{
    return configDir() + "/versions";
}

static QString sessionFile()
{
    return configDir() + "/session.json";
}

static QString indexFile()
{
    return configDir() + "/index.json";
}

static QString trashIndexFile()
{
    return configDir() + "/trash_index.json";
}

static QString recentFile()
{
    return configDir() + "/recent.json";
}

static QString windowFile()
{
    return configDir() + "/window.json";
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

// The "pinned" flag is owned by setPinned(), not by the tab snapshots that
// rewrite the rest of an index entry: carry it over.
static QJsonObject mergedMeta(const QJsonObject &index, const TabSnapshot &info)
{
    QJsonObject meta = metaToJson(info.filePath, info.defaultName, info.modified);
    if (index.value(info.id).toObject().value("pinned").toBool(false))
        meta["pinned"] = true;
    if (index.value(info.id).toObject().value("remember").toBool(false))
        meta["remember"] = true;
    return meta;
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
        if (info.cursor >= 0) {
            // où l'on travaillait dans la note (session.json seulement, pas index.json)
            entry["cursor"] = info.cursor;
            entry["scroll"] = info.scroll;
            if (info.anchor >= 0 && info.anchor != info.cursor)
                entry["anchor"] = info.anchor;
        }
        entries.append(entry);

        index[info.id] = mergedMeta(index, info);
    }

    QJsonObject sessionObj;
    sessionObj["active_id"] = toJsonOrNull(activeId);
    sessionObj["tabs"] = entries;
    writeJsonObject(sessionFile(), sessionObj);

    writeJsonObject(indexFile(), index);

    saveHistories(tabs);
}

// Un fichier `history/<id>.json` par onglet ouvert ayant un historique annuler/
// rétablir ; les autres (onglets fermés depuis, historique vide) sont supprimés.
static void saveHistories(const QVector<TabSnapshot> &tabs)
{
    QSet<QString> wanted;
    for (const TabSnapshot &info : tabs) {
        if (info.history.isEmpty())
            continue;
        wanted.insert(info.id);
        QDir().mkpath(historyDir());
        QFile f(historyDir() + "/" + info.id + ".json");
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            f.write(QJsonDocument(info.history).toJson(QJsonDocument::Compact));
    }
    const QStringList names = QDir(historyDir()).entryList({"*.json"}, QDir::Files);
    for (const QString &name : names) {
        if (!wanted.contains(name.chopped(5)))
            QFile::remove(historyDir() + "/" + name);
    }
}

QJsonObject loadHistory(const QString &id)
{
    return loadJsonObject(historyDir() + "/" + id + ".json");
}

void saveDraft(const TabSnapshot &info)
{
    QDir().mkpath(draftsDir());
    writeTextFile(draftsDir() + "/" + info.id + ".txt", info.content);

    QJsonObject index = loadJsonObject(indexFile());
    index[info.id] = mergedMeta(index, info);
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
        if (entry.contains("cursor")) {
            snap.cursor = entry.value("cursor").toInt(-1);
            snap.scroll = entry.value("scroll").toInt(0);
            if (entry.contains("anchor"))
                snap.anchor = entry.value("anchor").toInt(-1);
        }
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
        entry.pinned = meta.value("pinned").toBool(false);
        entry.remember = meta.value("remember").toBool(false);
        entry.mtimeMs = fi.lastModified().toMSecsSinceEpoch();
        items.append(entry);
    }

    std::sort(items.begin(), items.end(), [](const DraftEntry &a, const DraftEntry &b) {
        return a.mtimeMs > b.mtimeMs;
    });
    return items;
}

static QJsonArray loadRecentArray()
{
    return loadJsonObject(recentFile()).value("recent").toArray();
}

int recentLimit()
{
    const QJsonValue v = loadJsonObject(recentFile()).value("limit");
    if (v.isDouble() && v.toDouble() == static_cast<int>(v.toDouble()))
        return qBound(kMinRecent, v.toInt(), kMaxRecent);
    return kDefaultRecent;
}

static void saveRecentArray(const QJsonArray &entries, int limit = -1)
{
    QDir().mkpath(configDir());
    QJsonObject obj; // la limite est relue AVANT d'écrire (l'écriture remplace le fichier)
    obj["limit"] = limit < 0 ? recentLimit() : limit;
    obj["recent"] = entries;
    writeJsonObject(recentFile(), obj);
}

void setRecentLimit(int limit)
{
    limit = qBound(kMinRecent, limit, kMaxRecent);
    QJsonArray kept;
    for (const QJsonValue &v : loadRecentArray()) {
        if (kept.size() < limit)
            kept.append(v);
    }
    saveRecentArray(kept, limit);
}

void addRecent(const DraftEntry &entry)
{
    const int limit = recentLimit();
    QJsonArray entries;
    QJsonObject fresh;
    fresh["id"] = entry.id;
    fresh["file_path"] = toJsonOrNull(entry.filePath);
    fresh["default_name"] = toJsonOrNull(entry.defaultName);
    entries.append(fresh);
    for (const QJsonValue &v : loadRecentArray()) {
        if (v.toObject().value("id").toString() != entry.id && entries.size() < limit)
            entries.append(v);
    }
    saveRecentArray(entries);
}

void removeRecent(const QString &draftId)
{
    const QJsonArray entries = loadRecentArray();
    QJsonArray kept;
    for (const QJsonValue &v : entries) {
        if (v.toObject().value("id").toString() != draftId)
            kept.append(v);
    }
    if (kept.size() != entries.size())
        saveRecentArray(kept);
}

void clearRecent()
{
    if (!loadRecentArray().isEmpty())
        saveRecentArray(QJsonArray());
}

QVector<DraftEntry> listRecent()
{
    QSet<QString> drafts;
    for (const DraftEntry &d : listDrafts())
        drafts.insert(d.id);
    QVector<DraftEntry> result;
    for (const QJsonValue &v : loadRecentArray()) {
        const QJsonObject o = v.toObject();
        DraftEntry e;
        e.id = o.value("id").toString();
        if (e.id.isEmpty())
            continue;
        e.filePath = fromJsonOrEmpty(o.value("file_path"));
        e.defaultName = fromJsonOrEmpty(o.value("default_name"));
        const bool valid = (!e.filePath.isEmpty() && isExternalFile(e.filePath)) ? QFileInfo::exists(e.filePath)
                                                                                : drafts.contains(e.id);
        if (valid)
            result.append(e);
    }
    return result;
}

bool isExternalFile(const QString &filePath)
{
    if (filePath.isEmpty())
        return false;
    const QString path = QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());
    const QString root = QDir::cleanPath(QFileInfo(configDir()).absoluteFilePath());
    return !(path == root || path.startsWith(root + "/"));
}

DraftEntry findDraftForPath(const QString &filePath)
{
    for (const DraftEntry &entry : listDrafts()) { // newest first
        if (entry.filePath == filePath)
            return entry;
    }
    return DraftEntry();
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
    removeRecent(draftId);
}

bool isPinned(const QString &draftId)
{
    return loadJsonObject(indexFile()).value(draftId).toObject().value("pinned").toBool(false);
}

void setPinned(const QString &draftId, bool pinned)
{
    QJsonObject index = loadJsonObject(indexFile());
    QJsonObject entry = index.value(draftId).toObject();
    if (pinned)
        entry["pinned"] = true;
    else
        entry.remove("pinned");
    index[draftId] = entry;
    writeJsonObject(indexFile(), index);
}

bool isRemembered(const QString &draftId)
{
    return loadJsonObject(indexFile()).value(draftId).toObject().value("remember").toBool(false);
}

void setRemembered(const QString &draftId, bool remembered)
{
    QJsonObject index = loadJsonObject(indexFile());
    QJsonObject entry = index.value(draftId).toObject();
    if (remembered)
        entry["remember"] = true;
    else
        entry.remove("remember");
    index[draftId] = entry;
    writeJsonObject(indexFile(), index);
}

void renameDraft(const QString &draftId, const QString &newDefaultName)
{
    QJsonObject index = loadJsonObject(indexFile());
    QJsonObject entry = index.value(draftId).toObject();
    entry["default_name"] = newDefaultName;
    index[draftId] = entry;
    writeJsonObject(indexFile(), index);
}

void trashDraft(const QString &draftId)
{
    QDir().mkpath(trashDir());
    const QString src = draftsDir() + "/" + draftId + ".txt";
    const QString dst = trashDir() + "/" + draftId + ".txt";
    QFile::remove(dst);
    if (!QFile::rename(src, dst))
        return;

    QJsonObject index = loadJsonObject(indexFile());
    QJsonObject meta = index.value(draftId).toObject();
    index.remove(draftId);
    writeJsonObject(indexFile(), index);
    removeRecent(draftId);

    meta["deleted_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    QJsonObject trashIndex = loadJsonObject(trashIndexFile());
    trashIndex[draftId] = meta;
    writeJsonObject(trashIndexFile(), trashIndex);
}

QVector<TrashEntry> listTrash()
{
    const QJsonObject trashIndex = loadJsonObject(trashIndexFile());
    QVector<TrashEntry> items;
    for (auto it = trashIndex.constBegin(); it != trashIndex.constEnd(); ++it) {
        const QJsonObject meta = it.value().toObject();
        TrashEntry entry;
        entry.id = it.key();
        entry.filePath = fromJsonOrEmpty(meta.value("file_path"));
        entry.defaultName = fromJsonOrEmpty(meta.value("default_name"));
        entry.deletedAt = fromJsonOrEmpty(meta.value("deleted_at"));
        items.append(entry);
    }
    std::sort(items.begin(), items.end(), [](const TrashEntry &a, const TrashEntry &b) {
        return a.deletedAt > b.deletedAt;
    });
    return items;
}

void restoreDraft(const QString &draftId)
{
    const QString src = trashDir() + "/" + draftId + ".txt";
    const QString dst = draftsDir() + "/" + draftId + ".txt";
    QDir().mkpath(draftsDir());
    QFile::remove(dst);
    if (!QFile::rename(src, dst))
        return;

    QJsonObject trashIndex = loadJsonObject(trashIndexFile());
    QJsonObject meta = trashIndex.value(draftId).toObject();
    trashIndex.remove(draftId);
    writeJsonObject(trashIndexFile(), trashIndex);
    meta.remove("deleted_at");

    QJsonObject index = loadJsonObject(indexFile());
    index[draftId] = meta;
    writeJsonObject(indexFile(), index);
}

void purgeDraft(const QString &draftId)
{
    QFile::remove(trashDir() + "/" + draftId + ".txt");
    QJsonObject trashIndex = loadJsonObject(trashIndexFile());
    if (trashIndex.contains(draftId)) {
        trashIndex.remove(draftId);
        writeJsonObject(trashIndexFile(), trashIndex);
    }
}

static void pruneVersions(const QString &draftId)
{
    const QString dir = versionsDir() + "/" + draftId;
    QStringList names = QDir(dir).entryList(QStringList() << "*.txt", QDir::Files, QDir::Name | QDir::Reversed);
    for (int i = kMaxVersions; i < names.size(); ++i)
        QFile::remove(dir + "/" + names[i]);
}

void saveVersion(const QString &draftId, const QString &content)
{
    const QString dir = versionsDir() + "/" + draftId;
    QDir().mkpath(dir);
    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMddTHHmmsszzz");
    writeTextFile(dir + "/" + stamp + ".txt", content);
    pruneVersions(draftId);
}

void mergeVersions(const QString &fromId, const QString &toId)
{
    const QString src = versionsDir() + "/" + fromId;
    if (!QDir(src).exists())
        return;
    const QString dst = versionsDir() + "/" + toId;
    QDir().mkpath(dst);
    for (const QString &name : QDir(src).entryList(QDir::Files)) {
        QFile::remove(dst + "/" + name);
        QFile::rename(src + "/" + name, dst + "/" + name);
    }
    QDir().rmdir(src);
    pruneVersions(toId);
}

QStringList listVersions(const QString &draftId)
{
    const QString dir = versionsDir() + "/" + draftId;
    if (!QDir(dir).exists())
        return {};
    QStringList names = QDir(dir).entryList(QStringList() << "*.txt", QDir::Files, QDir::Name | QDir::Reversed);
    QStringList stamps;
    for (const QString &name : names)
        stamps.append(name.left(name.length() - 4));
    return stamps;
}

QString readVersion(const QString &draftId, const QString &stamp)
{
    QFile f(versionsDir() + "/" + draftId + "/" + stamp + ".txt");
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromUtf8(f.readAll());
}

void saveWindowState(int width, int height, const QList<int> &splitterSizes, int x, int y, bool wordWrap,
                     bool markdownPreview, bool sidebarVisible)
{
    QDir().mkpath(configDir());
    QJsonObject obj;
    obj["width"] = width;
    obj["height"] = height;
    QJsonArray sizesArray;
    for (int s : splitterSizes)
        sizesArray.append(s);
    obj["splitter_sizes"] = sizesArray;
    obj["x"] = x;
    obj["y"] = y;
    obj["word_wrap"] = wordWrap;
    obj["markdown_preview"] = markdownPreview;
    obj["sidebar_visible"] = sidebarVisible;
    writeJsonObject(windowFile(), obj);
}

WindowState loadWindowState()
{
    WindowState state;
    QFile f(windowFile());
    if (!f.open(QIODevice::ReadOnly))
        return state;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return state;

    const QJsonObject obj = doc.object();
    state.width = obj.value("width").toInt(0);
    state.height = obj.value("height").toInt(0);
    for (const QJsonValue &v : obj.value("splitter_sizes").toArray())
        state.splitterSizes.append(v.toInt());
    state.hasPosition = obj.contains("x") && obj.contains("y");
    if (state.hasPosition) {
        state.x = obj.value("x").toInt(0);
        state.y = obj.value("y").toInt(0);
    }
    state.hasSidebar = obj.contains("sidebar_visible");
    state.sidebarVisible = obj.value("sidebar_visible").toBool(true);
    state.hasWordWrap = obj.contains("word_wrap");
    state.wordWrap = obj.value("word_wrap").toBool(true);
    state.hasMarkdownPreview = obj.contains("markdown_preview");
    state.markdownPreview = obj.value("markdown_preview").toBool(false);
    state.valid = state.width > 0 && state.height > 0;
    return state;
}

} // namespace Session
