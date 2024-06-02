#!/usr/bin/env bash
src="leiden-communities-openmp-dynamic"
out="$HOME/Logs/$src$1.log"
ulimit -s unlimited
printf "" > "$out"

# Download program
if [[ "$DOWNLOAD" != "0" ]]; then
  rm -rf $src
  git clone https://github.com/puzzlef/$src
  cd $src
  git checkout input-temporal
fi

# Fixed config
: "${TYPE:=float}"
: "${MAX_THREADS:=64}"
: "${REPEAT_BATCH:=1}"
: "${REPEAT_METHOD:=1}"
# Parameter sweep for batch (randomly generated)
: "${BATCH_UNIT:=%}"
: "${BATCH_LENGTH:=100}"
: "${BATCH_DELETIONS_BEGIN:=0.00000005}"
: "${BATCH_DELETIONS_END:=0.05}"
: "${BATCH_DELETIONS_STEP:=*=10}"
: "${BATCH_INSERTIONS_BEGIN:=0.00000005}"
: "${BATCH_INSERTIONS_END:=0.05}"
: "${BATCH_INSERTIONS_STEP:=*=10}"
# Parameter sweep for number of threads
: "${NUM_THREADS_MODE:=all}"
: "${NUM_THREADS_BEGIN:=64}"
: "${NUM_THREADS_END:=64}"
: "${NUM_THREADS_STEP:=*=2}"
# Define macros (dont forget to add here)
DEFINES=(""
"-DTYPE=$TYPE"
"-DMAX_THREADS=$MAX_THREADS"
"-DREPEAT_BATCH=$REPEAT_BATCH"
"-DREPEAT_METHOD=$REPEAT_METHOD"
"-DBATCH_UNIT=\"$BATCH_UNIT\""
"-DBATCH_LENGTH=$BATCH_LENGTH"
"-DBATCH_DELETIONS_BEGIN=$BATCH_DELETIONS_BEGIN"
"-DBATCH_DELETIONS_END=$BATCH_DELETIONS_END"
"-DBATCH_DELETIONS_STEP=$BATCH_DELETIONS_STEP"
"-DBATCH_INSERTIONS_BEGIN=$BATCH_INSERTIONS_BEGIN"
"-DBATCH_INSERTIONS_END=$BATCH_INSERTIONS_END"
"-DBATCH_INSERTIONS_STEP=$BATCH_INSERTIONS_STEP"
"-DNUM_THREADS_MODE=\"$NUM_THREADS_MODE\""
"-DNUM_THREADS_BEGIN=$NUM_THREADS_BEGIN"
"-DNUM_THREADS_END=$NUM_THREADS_END"
"-DNUM_THREADS_STEP=$NUM_THREADS_STEP"
)

# Compile
g++ ${DEFINES[*]} -std=c++17 -O3 -fopenmp main.cxx

# Run on each temporal graph, with specified batch fraction
runEach() {
stdbuf --output=L ./a.out ~/Data/sx-mathoverflow.txt    248180   506550   239978   "$1" 2>&1 | tee -a "$out"
stdbuf --output=L ./a.out ~/Data/sx-askubuntu.txt       1593160  964437   596933   "$1" 2>&1 | tee -a "$out"
stdbuf --output=L ./a.out ~/Data/sx-superuser.txt       1940850  1443339  924886   "$1" 2>&1 | tee -a "$out"
stdbuf --output=L ./a.out ~/Data/wiki-talk-temporal.txt 11401490 7833140  3309592  "$1" 2>&1 | tee -a "$out"
stdbuf --output=L ./a.out ~/Data/sx-stackoverflow.txt   26019770 63497050 36233450 "$1" 2>&1 | tee -a "$out"
}

# Run with different batch fractions
runEach "0.00001"
runEach "0.0001"
runEach "0.001"

# Signal completion
curl -X POST "https://maker.ifttt.com/trigger/puzzlef/with/key/${IFTTT_KEY}?value1=$src$1"
