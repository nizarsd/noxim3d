#!/usr/bin/env bash
set -euo pipefail

X=$1
Y=$2
Z=$3
PIR=$4
SEL=${5:-dp}
SEED=${6:-2}
BUFFER=${7:-16}

NODES=$((X * Y * Z))
DIAMETER=$(((X - 1) + (Y - 1) + (Z - 1)))
CINTERVAL=$((2 * NODES * (DIAMETER + 3)))
WARMUP=$((5 * CINTERVAL))
SIM=$((20 * CINTERVAL))

./noxim \
  -dimx "$X" -dimy "$Y" -dimz "$Z" \
  -buffer "$BUFFER" \
  -routing fullyadaptive \
  -sel "$SEL" \
  -cinterval "$CINTERVAL" \
  -warmup "$WARMUP" -sim "$SIM" \
  -pir "$PIR" poisson \
  -traffic transpose1 \
  -seed "$SEED"