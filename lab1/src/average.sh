if [ $# -eq 0 ]; then  
    echo "Ошибка: нет входных аргументов: "$#
    exit 1  
fi  
sum=0
count=$#
for argument in $@; do

    if ! [[ "$argument" =~ ^-?[0-9]+(\.[0-9]+)?$ ]]; then  
        echo "Ошибка: аргумент '$argument' не является числом"  
        exit 1  
    fi  
      
    # Суммируем числа  
    sum=$(($sum + $argument)) 
done
average=$(($sum / $count))

echo "Среднее арифмитическое " $average