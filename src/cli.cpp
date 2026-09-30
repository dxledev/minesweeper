#include "cli.h"
#include "core/storage.h"
#include "core/game.h"
#include "core/theme.h"
#include "core/theme_source.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QTextStream>
#include <stdexcept>

namespace minesweeper {
namespace {

void requireCount(const QStringList &arguments, int count) {
    if (arguments.size() != count)
        throw std::invalid_argument("Invalid arguments. Run minesweeper --help for usage.");
}

}

Options parseOptions(QStringList arguments) {
    Options result;
    for (int index = 0; index < arguments.size(); ++index) {
        const QString argument = arguments[index];
        if (argument == "--config-dir" || argument == "--difficulty") {
            if (index + 1 >= arguments.size() || arguments[index + 1].startsWith("--"))
                throw std::invalid_argument((argument + " requires a value").toStdString());
            const auto value = arguments[++index];
            if (argument == "--config-dir")
                result.configDirectory = QDir(value).absolutePath();
            else {
                parseDifficulty(value.toStdString());
                result.difficulty = value;
            }
        } else if (argument == "--dry-run") {
            result.dryRun = true;
        } else if (argument == "--help" || argument == "-h") {
            result.help = true;
        } else if (argument == "--version") {
            result.version = true;
        } else if (argument.startsWith('-')) {
            throw std::invalid_argument(("Unknown option: " + argument).toStdString());
        } else {
            result.arguments.append(argument);
        }
    }
    if (result.configDirectory.isEmpty())
        result.configDirectory = defaultConfigDirectory();
    return result;
}

void printHelp() {
    QTextStream(stdout) << R"(Minesweeper — a portable C++ desktop game

Usage:
  minesweeper [--difficulty easy|intermediate|expert] [--config-dir DIRECTORY]
  minesweeper theme list
  minesweeper theme path
  minesweeper theme show
  minesweeper theme preset forest|paper|slate|rose [--dry-run]
  minesweeper theme set 'accent=#b6e3c6' 'background=#101819' [--dry-run]
  minesweeper theme follow file FILE_OR_DIRECTORY [--dry-run]
  minesweeper theme import FILE [--dry-run]
  minesweeper theme export FILE [--dry-run]

Theme commands write theme.json atomically; open windows reload immediately.
All commands accept --config-dir DIRECTORY (or MINESWEEPER_CONFIG_DIR).
--dry-run prints the proposed JSON without writing any files.
Palettes may use the native JSON schema, flat JSON colors, or QML color properties.
Preset, set, and import stop following an external palette.
Colors use #RRGGBB. Run theme show for the full schema.
)";

}

int runThemeCommand(const Options &options) {
    if (!options.difficulty.isEmpty())
        throw std::invalid_argument("--difficulty applies to the game, not theme commands");
    auto arguments = options.arguments;
    if (arguments.isEmpty() || arguments.takeFirst() != "theme" || arguments.isEmpty())
        throw std::invalid_argument("Expected a theme subcommand. Run minesweeper --help.");
    const QString command = arguments.takeFirst();
    const QString path = options.configDirectory + "/theme.json";
    QTextStream output(stdout);
    if (command == "list") {
        requireCount(arguments, 0);
        output << presetNames().join('\n') << '\n';
        return 0;
    }
    if (command == "path") {
        requireCount(arguments, 0);
        output << path << '\n';
        return 0;
    }
    const auto current = [&] {
        return QFileInfo::exists(path) ? loadTheme(path) : presetTheme("forest");
    };
    if (command == "show") {
        requireCount(arguments, 0);
        output << QJsonDocument(current().toJson()).toJson();
        return 0;
    }
    Theme next;
    QString destination = path;
    if (command == "preset") {
        requireCount(arguments, 1);
        next = presetTheme(arguments[0]);
    } else if (command == "set") {
        next = setColors(current(), arguments);
    } else if (command == "follow") {
        if (arguments.isEmpty() || arguments.size() > 2)
            throw std::invalid_argument("Expected theme follow SOURCE [PATH]");
        next = followTheme(arguments[0], arguments.value(1));
        if (QFileInfo(next.sourcePath).canonicalFilePath() == QFileInfo(path).canonicalFilePath()
            || QFileInfo(next.sourcePath).absoluteFilePath() == QFileInfo(path).absoluteFilePath())
            throw std::invalid_argument("A theme cannot follow its own output file");
    } else if (command == "import") {
        requireCount(arguments, 1);
        next = importPalette(QFileInfo(arguments[0]).absoluteFilePath());
        next.sourceKind.clear();
        next.sourcePath.clear();
    } else if (command == "export") {
        requireCount(arguments, 1);
        next = current();
        destination = QFileInfo(arguments[0]).absoluteFilePath();
    } else {
        throw std::invalid_argument(("Unknown theme command: " + command).toStdString());
    }
    if (options.dryRun)
        output << QJsonDocument(next.toJson()).toJson();
    else {
        writeJson(destination, next.toJson());
        output << destination << '\n';
    }
    return 0;
}

}
