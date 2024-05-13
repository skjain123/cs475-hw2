clear
g++   proj02.cpp  -DNUMT=$t -DNUMTRIALS=$n  -o proj02  -lm  -fopenmp
    #./proj02
    ./proj02 > proj02.csv 2>&1