#!/usr/bin/env bash
set -euo pipefail

BIN=${BIN:-./noxim}

DIMX=${DIMX:-5}
DIMY=${DIMY:-5}
DIMZ=${DIMZ:-3}

ROUTING=${ROUTING:-oddeven}
TRAFFIC=${TRAFFIC:-transpose1}

#PIR_LIST=${PIR_LIST:-"0.016 0.018 0.02 0.025 0.03 0.035 0.04"}

# PIR_LIST=${PIR_LIST:-"0.006 0.008 0.010 0.012 0.014"}
 PIR_LIST=${PIR_LIST:-"0.016 0.018 0.020 0.025 0.030 0.035 0.040"}
SEEDS=${SEEDS:-"2 6 10"}

OUTDIR=${OUTDIR:-results_compare_dp_bufferlevel}

# ------------------------------------------------------------
# DP-aware timing
# ------------------------------------------------------------

NUM_NODES=$((DIMX * DIMY * DIMZ))
DIAMETER=$(((DIMX - 1) + (DIMY - 1) + (DIMZ - 1)))

DP_DWELL=${DP_DWELL:-$((DIAMETER + 3))}
DP_CYCLE=$((2*NUM_NODES * DP_DWELL))

WARMUP_DP_CYCLES=${WARMUP_DP_CYCLES:- 2}
SIM_DP_CYCLES=${SIM_DP_CYCLES:-20}

MIN_WARMUP=${MIN_WARMUP:- 1000}
MIN_SIM=${MIN_SIM:-5000}

AUTO_WARMUP=$((WARMUP_DP_CYCLES * DP_CYCLE))
AUTO_SIM=$((SIM_DP_CYCLES * DP_CYCLE))

if (( AUTO_WARMUP < MIN_WARMUP )); then
    AUTO_WARMUP=$MIN_WARMUP
fi

if (( AUTO_SIM < MIN_SIM )); then
    AUTO_SIM=$MIN_SIM
fi
DP_CINTERVALS_PER_CYCLE=${DP_CINTERVALS_PER_CYCLE:-1}

CINTERVAL=$((DP_CYCLE / DP_CINTERVALS_PER_CYCLE))

WARMUP=${WARMUP:-$AUTO_WARMUP}
SIM=${SIM:-$AUTO_SIM}


echo "Timing:"
echo "  NUM_NODES=$NUM_NODES"
echo "  DIAMETER=$DIAMETER"
echo "  DP_DWELL=$DP_DWELL"
echo "  DP_CYCLE=$DP_CYCLE"
echo "  WARMUP=$WARMUP"
echo "  SIM=$SIM"
echo "  CINTERVAL=$CINTERVAL"
echo

mkdir -p "$OUTDIR/logs"

CSV="$OUTDIR/summary.csv"
MEAN_CSV="$OUTDIR/summary_mean.csv"

echo "selection,pir,seed,avg_delay,avg_throughput,total_received,total_sent,total_energy,raw_log" > "$CSV"

extract_metric() {
    local pattern="$1"
    local file="$2"

    grep -iE "$pattern" "$file" \
        | tail -1 \
        | grep -Eo '[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?' \
        | tail -1 || true
}

run_one() {
    local sel="$1"
    local pir="$2"
    local seed="$3"

    local tag="sel_${sel}_pir_${pir}_seed_${seed}"
    local log="$OUTDIR/logs/${tag}.log"

    echo "Running: selection=$sel pir=$pir seed=$seed"

    if ! "$BIN" \
        -dimx "$DIMX" \
        -dimy "$DIMY" \
        -dimz "$DIMZ" \
        -routing "$ROUTING" \
        -sel "$sel" \
        -cinterval "$CINTERVAL" \
        -warmup "$WARMUP" \
        -sim "$SIM" \
        -pir "$pir" poisson \
        -traffic "$TRAFFIC" \
        -seed "$seed" \
        > "$log" 2>&1
    then
        echo "ERROR: run failed: selection=$sel pir=$pir seed=$seed"
        echo "$sel,$pir,$seed,ERROR,ERROR,ERROR,ERROR,ERROR,$log" >> "$CSV"
        return
    fi

    avg_delay=$(extract_metric "average.*delay|avg.*delay|global.*delay|packet.*delay" "$log")
    avg_throughput=$(extract_metric "average.*throughput|avg.*throughput|throughput" "$log")
    total_received=$(extract_metric "received.*packets|total.*received|received" "$log")
    total_sent=$(extract_metric "sent.*packets|total.*sent|sent" "$log")
    total_energy=$(extract_metric "total.*energy|energy" "$log")

    avg_delay=${avg_delay:-NA}
    avg_throughput=${avg_throughput:-NA}
    total_received=${total_received:-NA}
    total_sent=${total_sent:-NA}
    total_energy=${total_energy:-NA}

    echo "$sel,$pir,$seed,$avg_delay,$avg_throughput,$total_received,$total_sent,$total_energy,$log" >> "$CSV"
}

for pir in $PIR_LIST; do
    for seed in $SEEDS; do
        run_one "dp" "$pir" "$seed"
        run_one "bufferlevel" "$pir" "$seed"
    done
done

echo
echo "Done."
echo "Raw logs: $OUTDIR/logs"
echo "CSV:      $CSV"

awk -F, '
NR==1 { next }
$4 != "NA" && $4 != "ERROR" {
    key=$1 "," $2
    delay_sum[key]+=$4
    thr_sum[key]+=$5
    n[key]++
}
END {
    print "selection,pir,mean_avg_delay,mean_avg_throughput,n"
    for (key in n) {
        split(key,a,",")
        print a[1] "," a[2] "," delay_sum[key]/n[key] "," thr_sum[key]/n[key] "," n[key]
    }
}
' "$CSV" | sort -t, -k1,1 -k2,2n > "$MEAN_CSV"

echo "Mean CSV: $MEAN_CSV"

