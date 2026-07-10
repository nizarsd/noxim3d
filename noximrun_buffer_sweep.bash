#!/usr/bin/env bash
set -euo pipefail

BIN=${BIN:-./noxim}

DIMX=${DIMX:-5}
DIMY=${DIMY:-5}
DIMZ=${DIMZ:-3}

ROUTING=${ROUTING:-oddevenbalanced}
TRAFFIC=${TRAFFIC:-transpose1}

PIR_LIST=${PIR_LIST:-"0.015 0.023 0.024 0.025"}
SEEDS=${SEEDS:-"2 6 10"}

# Buffer-depth sweep [flits]
# Noxim option: -buffer N
BUFFER_LIST=${BUFFER_LIST:-"8 16 32"}

OUTDIR=${OUTDIR:-results_compare_dp_bufferlevel_buffer_sweep}

# ------------------------------------------------------------
# DP-aware timing
# ------------------------------------------------------------

NUM_NODES=$((DIMX * DIMY * DIMZ))
DIAMETER=$(((DIMX - 1) + (DIMY - 1) + (DIMZ - 1)))

# diameter + 1 convergence + 2 settle cycles
DP_DWELL=${DP_DWELL:-$((DIAMETER + 3))}

# Keep two-phase factor unless you explicitly remove it from DP scheduling.
DP_PHASES=${DP_PHASES:-2}
DP_CYCLE=$((DP_PHASES * NUM_NODES * DP_DWELL))

WARMUP_DP_CYCLES=${WARMUP_DP_CYCLES:-5}
SIM_DP_CYCLES=${SIM_DP_CYCLES:-20}

MIN_WARMUP=${MIN_WARMUP:-1000}
MIN_SIM=${MIN_SIM:-5000}

AUTO_WARMUP=$((WARMUP_DP_CYCLES * DP_CYCLE))
AUTO_SIM=$((SIM_DP_CYCLES * DP_CYCLE))

if (( AUTO_WARMUP < MIN_WARMUP )); then
    AUTO_WARMUP=$MIN_WARMUP
fi

if (( AUTO_SIM < MIN_SIM )); then
    AUTO_SIM=$MIN_SIM
fi

WARMUP=${WARMUP:-$AUTO_WARMUP}
SIM=${SIM:-$AUTO_SIM}
CINTERVAL=${CINTERVAL:-$DP_CYCLE}

echo "Timing:"
echo "  NUM_NODES=$NUM_NODES"
echo "  DIAMETER=$DIAMETER"
echo "  DP_DWELL=$DP_DWELL"
echo "  DP_PHASES=$DP_PHASES"
echo "  DP_CYCLE=$DP_CYCLE"
echo "  WARMUP=$WARMUP"
echo "  SIM=$SIM"
echo "  CINTERVAL=$CINTERVAL"
echo

echo "Sweep:"
echo "  BUFFER_LIST=$BUFFER_LIST"
echo "  PIR_LIST=$PIR_LIST"
echo "  SEEDS=$SEEDS"
echo

mkdir -p "$OUTDIR/logs"

CSV="$OUTDIR/summary.csv"
MEAN_CSV="$OUTDIR/summary_mean.csv"

echo "buffer,selection,pir,seed,avg_delay,avg_throughput,total_received,total_sent,total_energy,raw_log" > "$CSV"

extract_metric() {
    local pattern="$1"
    local file="$2"

    grep -iE "$pattern" "$file" \
        | tail -1 \
        | grep -Eo '[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?' \
        | tail -1 || true
}

run_one() {
    local buffer="$1"
    local sel="$2"
    local pir="$3"
    local seed="$4"

    local tag="buf_${buffer}_sel_${sel}_pir_${pir}_seed_${seed}"
    local log="$OUTDIR/logs/${tag}.log"

    echo "Running: buffer=$buffer selection=$sel pir=$pir seed=$seed"

    if ! "$BIN" \
        -dimx "$DIMX" \
        -dimy "$DIMY" \
        -dimz "$DIMZ" \
        -buffer "$buffer" \
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
        echo "ERROR: run failed: buffer=$buffer selection=$sel pir=$pir seed=$seed"
        echo "$buffer,$sel,$pir,$seed,ERROR,ERROR,ERROR,ERROR,ERROR,$log" >> "$CSV"
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

    echo "$buffer,$sel,$pir,$seed,$avg_delay,$avg_throughput,$total_received,$total_sent,$total_energy,$log" >> "$CSV"
}

for buffer in $BUFFER_LIST; do
    for pir in $PIR_LIST; do
        for seed in $SEEDS; do
            run_one "$buffer" "dp" "$pir" "$seed"
            run_one "$buffer" "bufferlevel" "$pir" "$seed"
        done
    done
done

echo
echo "Done."
echo "Raw logs: $OUTDIR/logs"
echo "CSV:      $CSV"

{
    echo "buffer,selection,pir,mean_avg_delay,mean_avg_throughput,mean_total_energy,n"
    awk -F, '
    NR==1 { next }
    $5 != "NA" && $5 != "ERROR" {
        key=$1 "," $2 "," $3
        delay_sum[key]+=$5
        thr_sum[key]+=$6
        energy_sum[key]+=$9
        n[key]++
    }
    END {
        for (key in n) {
            split(key,a,",")
            print a[1] "," a[2] "," a[3] "," delay_sum[key]/n[key] "," thr_sum[key]/n[key] "," energy_sum[key]/n[key] "," n[key]
        }
    }
    ' "$CSV" | sort -t, -k1,1n -k2,2 -k3,3n
} > "$MEAN_CSV"

echo "Mean CSV: $MEAN_CSV"

# Optional paired DP-vs-bufferlevel summary by buffer and PIR
COMPARE_CSV="$OUTDIR/summary_compare.csv"

{
    echo "buffer,pir,bufferlevel_delay,dp_delay,dp_delay_reduction_pct,bufferlevel_throughput,dp_throughput,bufferlevel_energy,dp_energy"
    awk -F, '
    NR==1 { next }
    {
        key=$1 "," $3
        sel=$2

        delay[key,sel]=$4
        thr[key,sel]=$5
        energy[key,sel]=$6
        seen[key]=1
    }
    END {
        for (key in seen) {
            bld=delay[key,"bufferlevel"]
            dpd=delay[key,"dp"]
            blt=thr[key,"bufferlevel"]
            dpt=thr[key,"dp"]
            ble=energy[key,"bufferlevel"]
            dpe=energy[key,"dp"]

            if (bld != "" && dpd != "" && bld > 0) {
                split(key,a,",")
                reduction=(bld-dpd)/bld*100.0
                print a[1] "," a[2] "," bld "," dpd "," reduction "," blt "," dpt "," ble "," dpe
            }
        }
    }
    ' "$MEAN_CSV" | sort -t, -k1,1n -k2,2n
} > "$COMPARE_CSV"

echo "Compare CSV: $COMPARE_CSV"
