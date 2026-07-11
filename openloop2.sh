#./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing dw_xy  -pir 0.008  poisson -seed 10 -verbose -1 -tmode 0  -hs 122 0.01 -hs 123 0.01 -#hs 128 0.01 -hs 129 0.01 -bwt 110  > results/dw_0_hs_008.m
#./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing dw_xy  -pir 0.01  poisson -seed 10  -verbose -1 -tmode 0  -hs 122 0.01 -hs 123 0.01 -#hs 128 0.01 -hs 129 0.01 -bwt 110  > results/dw_0_hs_01.m


#./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing oddeven -sel dp -pir 0.008  poisson -seed 10 -verbose -1   -hs 122 0.01 -hs 123 0.01 -hs 128 0.01 -hs 129 0.01  > results/moe_0_hs_008.m
#./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing oddeven -sel dp -pir 0.01  poisson -seed 10  -verbose -1   -hs 122 0.01 -hs 123 0.01 -hs 128 0.01 -hs 129 0.01  > results/moe_0_hs_01.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing dw_xy  -pir 0.008  poisson -seed 10   -verbose -1  -bwt 110  > results/dw_0_rand_008.m
./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing dw_xy  -pir 0.01  poisson -seed 10   -verbose -1  -bwt 110  > results/dw_0_rand_01.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing oddeven  -pir 0.008  poisson -seed 10 -sel dp  -verbose -1  > results/moe_0_rand_008.m
./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing oddeven  -pir 0.01  poisson -seed 10 -sel dp  -verbose -1   > results/moe_0_rand_01.m
