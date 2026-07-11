./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 300  -routing dwoddeven  -pir 0.008  poisson -seed 10 -verbose -1 -tmode 2  -tupper 75 -tlower 75 -sel dp > results/xxdwoenm2_75_rand.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 300  -routing dwoddeven  -pir 0.008  poisson -seed 10 -verbose -1 -tmode 2  -tupper 75 -tlower 75 -traffic transpose2  -sel dp > results/xxdwoenm2_75_trans2.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 300  -routing dwoddeven  -pir 0.008  poisson -seed 10 -verbose -1 -tmode 2  -tupper 75 -tlower 75 -sel dp  -hs 122 0.01 -hs 123 0.01 -hs 128 0.01 -hs 129 0.01  > results/xxdwoenm2_75_hs.m


