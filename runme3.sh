./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 300  -routing dwoddeven  -pir 0.008  poisson -seed 10 -verbose -1 -tmode 2  -tupper 80 -tlower 80 -sel dp > results/dwoenm2_80_rand.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 300  -routing dwoddeven  -pir 0.008  poisson -seed 10 -verbose -1 -tmode 2  -tupper 80 -tlower 80 -traffic transpose2  -sel dp > results/dwoenm2_80_trans2.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 300  -routing dwoddeven  -pir 0.008  poisson -seed 10 -verbose -1 -tmode 2  -tupper 80 -tlower 80 -sel dp  -hs 122 0.01 -hs 123 0.01 -hs 128 0.01 -hs 129 0.01  > results/dwoenm2_80_hs.m


./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 300  -routing oddevennm  -pir 0.008  poisson -seed 10 -verbose -1 -tmode 2  -tupper 80 -tlower 80 -sel dp > results/oenm2_80_rand.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 300  -routing oddevennm  -pir 0.008  poisson -seed 10 -verbose -1 -tmode 2  -tupper 80 -tlower 80 -traffic transpose2  -sel dp > results/oenm2_80_trans2.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 300  -routing oddevennm  -pir 0.008  poisson -seed 10 -verbose -1 -tmode 2  -tupper 80 -tlower 80 -sel dp  -hs 122 0.01 -hs 123 0.01 -hs 128 0.01 -hs 129 0.01  > results/oenm2_80_hs.m


./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 300  -routing dw_xy  -pir 0.008  poisson -seed 10 -verbose -1 -tmode 1  -tupper 80 -tlower 80 -bwt 110 > results/dw1_80_rand.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 300  -routing dw_xy  -pir 0.008  poisson -seed 10 -verbose -1 -tmode 1  -tupper 80 -tlower 80 -traffic transpose2  -bwt 80 > results/dw1_80_trans2.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 300  -routing dw_xy  -pir 0.008  poisson -seed 10 -verbose -1 -tmode 1  -tupper 80 -tlower 80 -hs 122 0.01 -hs 123 0.01 -hs 128 0.01 -hs 129 0.01 -bwt 110  > results/dw1_80_hs.m

