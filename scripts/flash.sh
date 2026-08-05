#!/usr/bin/env bash
# Build one board's example context and flash it over the debug station
# (tools/debug-station/).
#
#   scripts/flash_example.sh <btc|exp1|exp2|exp3> [Debug|Release]
#
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Assumes this script is in a scripts/ folder at the root of your sensor library
LIB_ROOT="$(cd "${HERE}/.." && pwd)"
HOST="${BOLT_STATION:-bolt-station.local}"
GDB="${GDB:-gdb-multiarch}"
NODE="${1:-}"
CFG="${2:-Debug}"

# In the sensor library, the cproject is always named "example"
PROJ="example"

usage() { echo "usage: flash_example.sh <bolt-btc|bolt-exp1|bolt-exp2|bolt-exp3> [Debug|Release]"; exit 2; }
command -v "${GDB}" >/dev/null || { echo "error: ${GDB} not found"; exit 1; }

# Single probe: one gdb port; WHICH board gets flashed is decided by where
# the probe physically sits - put it on the board you name here
PORT=3401

# Map the node arguments to the specific target names used in examples.csolution.yml.
# Adjust the exp1/exp2/exp3 strings if they use the longer suffixes (e.g. mission-bolt-exp1-space-disco)
declare -A MAP=(
    [bolt-btc]="mission-bolt-btc"
    [bolt-exp1]="mission-bolt-exp1"
    [bolt-exp2]="mission-bolt-exp2"
    [bolt-exp3]="mission-bolt-exp3"
)

wait_for_station() {
    for _ in $(seq 1 15); do
        if (exec 3<>"/dev/tcp/${HOST}/${PORT}") 2>/dev/null; then
            exec 3>&- 3<&-
            return 0
        fi
        echo "   station ${HOST}:${PORT} not ready (probe attached + board powered?) - retrying"
        sleep 2
    done
    echo "error: station ${HOST}:${PORT} unreachable after 30 s"
    return 1
}

flash_one() {
    local node="$1" tgt port elf
    tgt="${MAP[${node}]}"
    port="${PORT}"
    
    # ELF path structure reflects the fixed project name
    elf="${LIB_ROOT}/examples/out/${PROJ}/${tgt}/${CFG}/${PROJ}.elf"

    # incremental cbuild is seconds; flashing a stale ELF costs a bench trip
    if command -v cbuild >/dev/null; then
        echo "== ${node}: cbuild ${PROJ}.${CFG}+${tgt}"
        # Point to examples.csolution.yml and use the PROJ variable for the context
        (cd "${LIB_ROOT}/examples/" && cbuild examples.csolution.yml --context "${PROJ}.${CFG}+${tgt}" >/dev/null) \
            || { echo "== ${node}: build FAILED"; return 1; }
    else
        echo "   note: cbuild not found - flashing the existing ELF as-is"
    fi
    [ -f "${elf}" ] || { echo "error: ${elf} missing"; return 1; }

    echo "== ${node}: ${elf#"${LIB_ROOT}"/} -> ${HOST}:${port} (probe must sit on ${node}!)"
    wait_for_station || return 1

    local noinit clear_state=()
    noinit="$(arm-none-eabi-objdump -h "${elf}" 2>/dev/null | awk '$2==".noinit" {print $4}')"
    if [ -n "${noinit}" ]; then
        clear_state=(-ex "set {unsigned int}0x${noinit} = 0")
    else
        echo "   note: no .noinit section found - mission state not cleared"
    fi

    "${GDB}" -batch -nx \
        -ex "target extended-remote ${HOST}:${port}" \
        -ex "monitor reset halt" \
        -ex "load" \
        "${clear_state[@]}" \
        -ex "compare-sections" \
        -ex "monitor reset run" \
        "${elf}" || { echo "== ${node}: FAILED"; return 1; }
    echo "== ${node}: flashed, mission state cleared, running in TEST"
}

case "${NODE}" in
bolt-btc | bolt-exp1 | bolt-exp2 | bolt-exp3) flash_one "${NODE}" ;;
*) usage ;;
esac
