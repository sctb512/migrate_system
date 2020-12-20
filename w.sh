#!/bin/bash

# echo "Please input the num "
num=30
sleep_great=36s
sleep_reset=36s
sleep_s=$sleep_great

# factorial=1
i=0
while [ "$num" -gt 0 ]
do
    # let "factorial= factorial+num"
    let "num--"
    let "i++"
    pkill python
    pkill pull
    echo "pkill main $i st"
    sleep 2s
    make cfw &
    sleep 2s
    
    pid=$(pidof pull)
    echo "pid: $pid"
    m=0
    while [ $m -lt 10 ]
    do
        if [ -z "$pid" ]
        then
        #     echo "start process_monitor.py"
        #     python /home/xy/process_monitor/process_monitor.py $pid 32 &
        # else
            make cfw &
            echo "pid: $pid"
        else
            break
        fi
        let "m++"
    done
    if [ $m == 10 ]
    then
        sleep_s=$sleep_reset
    else
        sleep_s=$sleep_great
    fi
    # echo "end process_monitor.py"
    echo "sleep: $sleep_s"
    sleep $sleep_s
done

pkill pull
echo "while end!!!"
# echo "The factorial is $factorial"
