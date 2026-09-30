#include "cli.h"
#include "core/game.h"
#include "core/storage.h"
#include "core/theme.h"
#include "ui/window.h"

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QTextStream>
#include <stdexcept>

int main(int argc, char **argv) {
    QCoreApplication::setApplicationName("minesweeper");
    QCoreApplication::setApplicationVersion("1.0.0");
    QCoreApplication::setOrganizationName("Minesweeper");
    try {
        QStringList arguments;
        for (int index = 1; index < argc; ++index) arguments.append(QString::fromLocal8Bit(argv[index]));
        const auto options = minesweeper::parseOptions(arguments);
        if (options.help || options.version || !options.arguments.isEmpty()) {
            QCoreApplication app(argc, argv);
            if (options.help) { minesweeper::printHelp(); return 0; }
            if (options.version) { QTextStream(stdout) << "Minesweeper 1.0.0\n"; return 0; }
            return minesweeper::runThemeCommand(options);
        }
        if (options.dryRun) throw std::invalid_argument("--dry-run applies to theme commands");
        if (!qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY") && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
            qputenv("QT_QPA_PLATFORM", "wayland");
        // The board uses raster painting, so it needs no GPU buffers.
        if (qEnvironmentVariableIsEmpty("QT_WAYLAND_CLIENT_BUFFER_INTEGRATION"))
            qputenv("QT_WAYLAND_CLIENT_BUFFER_INTEGRATION", "shm");
        QApplication app(argc, argv);
        app.setStyle("Fusion");
        app.setApplicationDisplayName("Minesweeper");
        app.setDesktopFileName("io.github.minesweeper");
        app.setWindowIcon(QIcon::fromTheme("io.github.minesweeper"));
        const auto theme = minesweeper::ensureTheme(options.configDirectory + "/theme.json");
        const auto difficulty = options.difficulty.isEmpty() ? minesweeper::Difficulty::Easy
            : minesweeper::parseDifficulty(options.difficulty.toStdString());
        minesweeper::Window window(options.configDirectory, theme, difficulty);
        window.show();
        return app.exec();
    } catch (const std::exception &error) {
        QTextStream(stderr) << "minesweeper: " << error.what() << '\n';
        return 1;
    }
}
