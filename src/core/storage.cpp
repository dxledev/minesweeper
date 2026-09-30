#include "storage.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <stdexcept>

namespace minesweeper {

QString defaultConfigDirectory() {
    const auto override = qEnvironmentVariable("MINESWEEPER_CONFIG_DIR");
    return override.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/minesweeper"
        : QDir(override).absolutePath();
}

QJsonObject readJson(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 1024 * 1024)
        throw std::runtime_error(("Cannot read JSON: " + path).toStdString());
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        throw std::invalid_argument(("Invalid JSON: " + path + ": " + error.errorString()).toStdString());
    return document.object();
}

void writeJson(const QString &path, const QJsonObject &data, QJsonDocument::JsonFormat format) {
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        throw std::runtime_error(("Cannot create config directory for: " + path).toStdString());
    QSaveFile file(path);
    const auto bytes = QJsonDocument(data).toJson(format);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        throw std::runtime_error(("Cannot save JSON: " + path + ": " + file.errorString()).toStdString());
}

}
