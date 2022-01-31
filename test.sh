things="bla@some.com;C12i21 gabriele@whist.com;none"

for one_thing in $(echo $things); do
    #echo $one_thing
    IN=$one_thing

    IFS=';' read -ra ADDR <<< "$IN"
    echo ${ADDR[0]} ${ADDR[1]}


done



