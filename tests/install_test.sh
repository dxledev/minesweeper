#!/usr/bin/env bash
set -euo pipefail

build_directory="$1"
installer="$2"
test_directory="$(/usr/bin/mktemp -d /tmp/minesweeper-install.XXXXXX)"
prefix="$test_directory/install with spaces"
config_directory="$test_directory/config"
apps_list="$test_directory/apps.list"
palette="$test_directory/input.json"
desktop_id=io.github.minesweeper

"$build_directory/minesweeper" theme preset paper --dry-run > "$palette"
printf 'icon-override = true\nexample.desktop|/icons/example.svg|Example\n' > "$apps_list"
/usr/bin/cp "$apps_list" "$test_directory/before.list"
arguments=(--build-dir "$build_directory" --prefix "$prefix" --config-dir "$config_directory"
           --apps-list "$apps_list" --palette "$palette")
bash "$installer" "${arguments[@]}" --dry-run > /dev/null
[[ ! -e "$prefix" && ! -e "$config_directory" ]]
/usr/bin/cmp "$apps_list" "$test_directory/before.list"
bash "$installer" "${arguments[@]}" > /dev/null
[[ -x "$prefix/bin/minesweeper" ]]
[[ -f "$prefix/bin/applications/$desktop_id.desktop" ]]
[[ "$(/usr/bin/readlink "$prefix/share/applications/$desktop_id.desktop")" == "$prefix/bin/applications/$desktop_id.desktop" ]]
[[ "$(< "$prefix/bin/applications/$desktop_id.desktop")" == *'Name=Minesweeper'* ]]
[[ "$(< "$prefix/bin/applications/$desktop_id.desktop")" == *"Exec=\"$prefix/bin/minesweeper\" --config-dir \"$config_directory\""* ]]
[[ "$(< "$config_directory/theme.json")" == *'"source"'* ]]
if [[ -x /usr/bin/desktop-file-validate ]]; then
    /usr/bin/desktop-file-validate "$prefix/bin/applications/$desktop_id.desktop"
fi
/usr/bin/head -n 2 "$apps_list" > "$test_directory/preserved.list"
/usr/bin/cmp "$test_directory/preserved.list" "$test_directory/before.list"
bash "$installer" "${arguments[@]}" > /dev/null
count=0
while IFS='|' read -r launcher rest; do
    if [[ "$launcher" == "$desktop_id.desktop" ]]; then count=$((count + 1)); fi
done < "$apps_list"
[[ "$count" == 1 ]]
/usr/bin/ln -s "$apps_list" "$test_directory/symlink.list"
if bash "$installer" "${arguments[@]}" --apps-list "$test_directory/symlink.list" > /dev/null 2>&1; then
    printf 'Installer must preserve symlinked app lists\n' >&2
    exit 1
fi
