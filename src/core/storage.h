#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace minesweeper {

QString defaultConfigDirectory();
QJsonObject readJson(const QString &path);
void writeJson(const QString &path, const QJsonObject &data,
               QJsonDocument::JsonFormat format = QJsonDocument::Indented);

}
