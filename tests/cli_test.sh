#!/usr/bin/env bash
set -euo pipefail

binary="$1"
test_directory="$(/usr/bin/mktemp -d /tmp/minesweeper-cli.XXXXXX)"
config_directory="$test_directory/config"

theme() {
    "$binary" --config-dir "$config_directory" theme "$@"
}

expect_failure() {
    if "$@" > "$test_directory/rejected" 2>&1; then
        printf 'Expected command to fail\n' >&2
        exit 1
    fi
}

unset DISPLAY WAYLAND_DISPLAY
theme preset paper --dry-run > "$test_directory/proposed.json"
[[ ! -e "$config_directory" ]]
theme list > "$test_directory/list"
theme show > "$test_directory/default.json"
[[ ! -e "$config_directory" ]]
[[ "$(theme path)" == "$config_directory/theme.json" ]]
theme preset paper > /dev/null
theme export "$test_directory/exported.json" > /dev/null
/usr/bin/cmp "$test_directory/proposed.json" "$test_directory/exported.json"
theme set 'accent=#112233' > /dev/null
[[ "$(theme show)" == *'"accent": "#112233"'* ]]
/usr/bin/cp "$config_directory/theme.json" "$test_directory/before.json"
theme preset rose --dry-run > /dev/null
/usr/bin/cmp "$config_directory/theme.json" "$test_directory/before.json"
expect_failure theme set 'accent=wrong'
expect_failure theme set 'unknown=#112233'
expect_failure "$binary" --difficulty impossible
expect_failure "$binary" --unknown
expect_failure "$binary" --dry-run
expect_failure theme follow file "$config_directory/theme.json"
/usr/bin/cmp "$config_directory/theme.json" "$test_directory/before.json"
theme import "$test_directory/exported.json" > /dev/null
theme follow file "$test_directory/exported.json" > /dev/null
[[ "$(theme show)" == *'"source"'* ]]
theme set 'accent=#abcdef' > /dev/null
[[ "$(theme show)" != *'"source"'* ]]
printf '{invalid}\n' > "$test_directory/invalid.json"
expect_failure theme import "$test_directory/invalid.json"
printf '{}\n' > "$config_directory/theme.json"
expect_failure theme show
[[ "$(< "$config_directory/theme.json")" == '{}' ]]
"$binary" --help > /dev/null
[[ "$("$binary" --version)" == 'Minesweeper 1.0.0' ]]
