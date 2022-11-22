#!/bin/zsh

# Array containing the input numbers for psort
declare -a input_numbers=(1000000 5000000 10000000 50000000 100000000)
# Array containing the input thread numbers for psort
declare -a input_threads=(2 3 5 9 13 16)


for i in "${input_numbers[@]}"; do
    TIME="0"
    # echo -n "seqsort $i: \t\t"
    COMMAND="./seqsort $i"
    # run three times and average the result
    for j in {1..3}; do
        TIME=$(($TIME + "$(eval $COMMAND | grep time | cut -d ' ' -f 4)"))
    done
    TIME=$(($TIME / 3))
    printf "%.3f" "$TIME"
    echo ""

    TIME="0"
    for j in "${input_threads[@]}"; do
        # echo -n "psort   $i   $j threads:\t"
        COMMAND="./psort $i $j"
        for j in {1..3}; do
            TIME=$(($TIME + "$(eval $COMMAND | grep time | cut -d ' ' -f 4)"))
        done
        TIME=$(($TIME / 3))
        printf "%.3f" "$TIME"
        echo ""
    done
    echo""
done
