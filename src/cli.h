#pragma once

#include <QStringList>

namespace minesweeper {

struct Options {
    QString configDirectory;
    QString difficulty;
    QStringList arguments;
    bool dryRun = false;
    bool help = false;
    bool version = false;
};

Options parseOptions(QStringList arguments);
void printHelp();
int runThemeCommand(const Options &options);

}
