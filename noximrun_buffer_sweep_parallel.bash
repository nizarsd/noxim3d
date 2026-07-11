#!/usr/bin/env bash
set -euo pipefail

# Parallel version of noximrun_buffer_sweep.bash.
#
# Runs the (buffer x pir x seed x selection) grid concurrently across cores.
# Each noxim run is single-threaded, CPU-bound, and deterministic (fixed -seed),
# so parallel execution produces per-run numbers identical to the sequential
# sweep -- only wall-clock time changes.
#
# Concurrency is controlled by JOBS (default: nproc). To avoid a race on the
# shared summary CSV, each job writes its own row file; rows are concatenated
# after all jobs finish.

BIN=${BIN:-./noxim}

DIMX=${DIMX:-5}
DIMY=${DIMY:-5}
DIMZ=${DIMZ:-3}

ROUTING=${ROUTING:-oddevenbalanced}
TRAFFIC=${TRAFFIC:-transpose1}

PIR_LIST=${PIR_LIST:-"0.015 0.023 0.024 0.025"}
SEEDS=${SEEDS:-"2 6 10"}
BUFFER_LIST=${BUFFER_LIST:-"8 16 32"}

OUTDIR=${OUTDIR:-results_compare_dp_bufferlevel_buffer_sweep}

# Parallelism: number of concurrent noxim processes. Default = core count.
JOBS=${JOBS:-$(nproc)}

# ------------------------------------------------------------
# DP-aware timing (identical derivation to the sequential script)
# ------------------------------------------------------------
NUM_NODES=$((DIMX * DIMY * DIMZ))
DIAMETER=$(((DIMX - 1) + (DIMY - 1) + (DIMZ - 1)))
DP_DWELL=${DP_DWELL:-$((DIAMETER + 3))}
DP_PHASES=${DP_PHASES:-2}
DP_CYCLE=$((DP_PHASES * NUM_NODES * DP_DWELL))
WARMUP_DP_CYCLES=${WARMUP_DP_CYCLES:-5}
SIM_DP_CYCLES=${SIM_DP_CYCLES:-20}
MIN_WARMUP=${MIN_WARMUP:-1000}
MIN_SIM=${MIN_SIM:-5000}
AUTO_WARMUP=$((WARMUP_DP_CYCLES * DP_CYCLE))
AUTO_SIM=$((SIM_DP_CYCLES * DP_CYCLE))
(( AUTO_WARMUP < MIN_WARMUP )) && AUTO_WARMUP=$MIN_WARMUP
(( AUTO_SIM < MIN_SIM )) && AUTO_SIM=$MIN_SIM
WARMUP=${WARMUP:-$AUTO_WARMUP}
SIM=${SIM:-$AUTO_SIM}
CINTERVAL=${CINTERVAL:-$DP_CYCLE}

echo "Timing:"
echo "  NUM_NODES=$NUM_NODES  DIAMETER=$DIAMETER  DP_DWELL=$DP_DWELL  DP_PHASES=$DP_PHASES"
echo "  DP_CYCLE=$DP_CYCLE  WARMUP=$WARMUP  SIM=$SIM  CINTERVAL=$CINTERVAL"
echo
echo "Sweep:"
echo "  BUFFER_LIST=$BUFFER_LIST"
echo "  PIR_LIST=$PIR_LIST"
echo "  SEEDS=$SEEDS"
echo "  JOBS=$JOBS (parallel)"
echo

mkdir -p "$OUTDIR/logs" "$OUTDIR/rows"
# Clear stale row files from a previous run of this OUTDIR.
find "$OUTDIR/rows" -maxdepth 1 -name '*.row' -delete 2>/dev/null || true

CSV="$OUTDIR/summary.csv"
MEAN_CSV="$OUTDIR/summary_mean.csv"
COMPARE_CSV="$OUTDIR/summary_compare.csv"

extract_metric() {
    local pattern="$1"
    local file="$2"
    grep -iE "$pattern" "$file" \
        | tail -1 \
        | grep -Eo '[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?' \
        | tail -1 || true
}

# One grid point. Writes a single CSV row to its own file (no shared-file race).
run_one() {
    local buffer="$1" sel="$2" pir="$3" seed="$4"
    local tag="buf_${buffer}_sel_${sel}_pir_${pir}_seed_${seed}"
    local log="$OUTDIR/logs/${tag}.log"
    local row="$OUTDIR/rows/${tag}.row"

    if ! "$BIN" \
        -dimx "$DIMX" -dimy "$DIMY" -dimz "$DIMZ" \
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
        echo "$buffer,$sel,$pir,$seed,ERROR,ERROR,ERROR,ERROR,ERROR,$log" > "$row"
        echo "  FAILED: $tag"
        return 0
    fi

    local avg_delay avg_throughput total_received total_sent total_energy
    avg_delay=$(extract_metric "average.*delay|avg.*delay|global.*delay|packet.*delay" "$log")
    avg_throughput=$(extract_metric "average.*throughput|avg.*throughput|throughput" "$log")
    total_received=$(extract_metric "received.*packets|total.*received|received" "$log")
    total_sent=$(extract_metric "sent.*packets|total.*sent|sent" "$log")
    total_energy=$(extract_metric "total.*energy|energy" "$log")

    echo "$buffer,$sel,$pir,$seed,${avg_delay:-NA},${avg_throughput:-NA},${total_received:-NA},${total_sent:-NA},${total_energy:-NA},$log" > "$row"
    echo "  done: $tag"
}

export -f run_one extract_metric
export BIN DIMX DIMY DIMZ ROUTING TRAFFIC CINTERVAL WARMUP SIM OUTDIR

# Emit the job list (one "buffer sel pir seed" per line) and run JOBS at a time.
{
    for buffer in $BUFFER_LIST; do
        for pir in $PIR_LIST; do
            for seed in $SEEDS; do
                echo "$buffer dp $pir $seed"
                echo "$buffer bufferlevel $pir $seed"
            done
        done
    done
} | xargs -P "$JOBS" -n 4 bash -c 'run_one "$@"' _

# ------------------------------------------------------------
# Assemble summary.csv from the per-job row files (deterministic order).
# ------------------------------------------------------------
{
    echo "buffer,selection,pir,seed,avg_delay,avg_throughput,total_received,total_sent,total_energy,raw_log"
    cat "$OUTDIR/rows/"*.row | sort -t, -k1,1n -k2,2 -k3,3n -k4,4n
} > "$CSV"

echo
echo "Done."
echo "Raw logs: $OUTDIR/logs"
echo "CSV:      $CSV"

# ------------------------------------------------------------
# Mean over seeds (identical aggregation to the sequential script).
# ------------------------------------------------------------
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

# ------------------------------------------------------------
# Paired DP-vs-bufferlevel comparison by buffer and PIR.
# ------------------------------------------------------------
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
