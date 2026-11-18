#!/usr/bin/env bash
set -euo pipefail

destination="$HOME/.vst3/ParametricNAM.vst3"
case "${1:-}" in
    --yes) ;;
    "")
        printf 'Install ParametricNAM VST3 in %s? [y/N] ' "$destination"
        read -r answer
        case "$answer" in [Yy]|[Yy][Ee][Ss]) ;; *) exit 0 ;; esac
        ;;
    *)
        printf 'Usage: bash %s [--yes]\n' "$0" >&2
        exit 2
        ;;
esac

case "$(uname -m)" in
    x86_64) ;;
    *) printf 'This installer requires Linux x86-64.\n' >&2; exit 1 ;;
esac

mkdir -p "$(dirname "$destination")"
work=$(mktemp -d "$(dirname "$destination")/.pnam-install.XXXXXX")
cleanup() {
    if [[ -e "$work/previous" || -L "$work/previous" ]]; then
        if [[ ! -e "$destination" && ! -L "$destination" ]]; then
            mv -T -- "$work/previous" "$destination" || return
        fi
    fi
    rm -rf -- "$work"
}
trap cleanup EXIT

payload=$(awk '/^__PNAM_PAYLOAD__$/ { print NR + 1; exit }' "$0")
[[ -n "$payload" ]]
tail -n +"$payload" "$0" | tar -xzf - -C "$work"
test -f "$work/ParametricNAM.vst3/Contents/x86_64-linux/ParametricNAM.so"
if [[ -e "$destination" || -L "$destination" ]]; then
    mv -T -- "$destination" "$work/previous"
fi
mv -T -- "$work/ParametricNAM.vst3" "$destination"
printf 'Installed ParametricNAM VST3. Rescan plug-ins in your DAW.\n'
exit 0
__PNAM_PAYLOAD__
