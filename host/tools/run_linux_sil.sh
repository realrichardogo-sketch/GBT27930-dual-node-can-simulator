#!/bin/sh
# tools/run_linux_sil.sh <build_dir> <evidence_dir> [ifname]
# Two-process SocketCAN SIL (Stage 6B). Runs charger and vehicle as separate
# processes on a vcan interface, captures the bus with candump -L, verifies
# the capture. Exit: 0 all scenarios passed, 1 failure, 77 interface missing
# (reported as NOT RUN by CTest - never as passed).
set -u
B="$1"; OUT="$2"; IF="${3:-vcan0}"
HERE=$(cd "$(dirname "$0")" && pwd)

# Presence check in this process's network namespace: netlink (ip) when
# available; /sys/class/net can show another namespace's interfaces.
if command -v ip >/dev/null 2>&1; then
    ip link show "$IF" >/dev/null 2>&1; present=$?
else
    [ -d "/sys/class/net/$IF" ]; present=$?
fi
if [ "$present" -ne 0 ]; then
    echo "SocketCAN interface '$IF' not present: two-process SIL NOT RUN."
    echo "Enable it once (requires root), then re-run:"
    echo "  sudo modprobe vcan"
    echo "  sudo ip link add dev $IF type vcan"
    echo "  sudo ip link set up $IF"
    exit 77
fi
command -v candump >/dev/null 2>&1 || { echo "candump (can-utils) not found"; exit 1; }
mkdir -p "$OUT"
ip -details link show "$IF" > "$OUT/interface.txt" 2>&1
fail=0

run() {  # name, charger args, vehicle args
    name="$1"; cargs="$2"; vargs="$3"; d="$OUT/$name"
    mkdir -p "$d"
    candump -L "$IF" > "$d/candump.log" 2> "$d/candump.err" &
    cd_pid=$!
    sleep 0.3
    # shellcheck disable=SC2086
    "$B/gbt_vehicle_node" --if "$IF" $vargs > "$d/vehicle.log" 2>&1 &
    v_pid=$!
    # shellcheck disable=SC2086
    "$B/gbt_charger_node" --if "$IF" $cargs > "$d/charger.log" 2>&1 &
    c_pid=$!
    wait "$c_pid"; c_rc=$?
    wait "$v_pid"; v_rc=$?
    sleep 0.2
    kill "$cd_pid" 2>/dev/null; wait "$cd_pid" 2>/dev/null
    python3 -I "$HERE/verify_canlog.py" "$d/candump.log" "$name" > "$d/verify.txt" 2>&1; ver=$?
    echo "$name: charger rc=$c_rc vehicle rc=$v_rc verify rc=$ver (pids $c_pid/$v_pid)" | tee "$d/summary.txt"
    grep RESULT "$d/charger.log" "$d/vehicle.log"; tail -1 "$d/verify.txt"
    if [ "$c_rc" -ne 0 ] || [ "$v_rc" -ne 0 ] || [ "$ver" -ne 0 ]; then fail=1; fi
}

# 1-5: handshake, configuration, BRM/BCP/BCS transport, charging, vehicle-initiated stop
run normal       "--expect end"                                  "--expect end --soc-target 520"
# 6: charger-initiated stop
run charger-stop "--expect end --stop-after-ms 1000"            "--expect end --soc-target 1000"
# 7: BCL timeout (vehicle stops sending BCL 1 s into charging)
run bcl-timeout  "--expect exit"                                  "--expect exit --soc-target 1000 --drop-bcl-after-ms 1000"
# 8: CCS timeout (charger stops sending CCS 1 s into charging)
run ccs-timeout  "--expect exit --drop-ccs-after-ms 1000"        "--expect exit --soc-target 1000"

[ "$fail" -eq 0 ] && echo "TWO-PROCESS SOCKETCAN SIL: PASS" || echo "TWO-PROCESS SOCKETCAN SIL: FAIL"
exit "$fail"
