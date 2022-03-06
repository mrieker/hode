set -x
while true
do
    sleep 1
    ./raspi-gpio set 7 dh
    sleep 1
    ./raspi-gpio set 7 dl
done
