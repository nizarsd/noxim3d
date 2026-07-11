./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing oddeven  -pir 0.002  poisson -seed 10   -verbose -1  -traffic transpose2 -sel dp > results/moe_0_trans2_002.m
./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing oddeven  -pir 0.004  poisson -seed 10   -verbose -1  -traffic transpose2 -sel dp > results/moe_0_trans2_004.m
./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing oddeven  -pir 0.006  poisson -seed 10   -verbose -1  -traffic transpose2 -sel dp > results/moe_0_trans2_006.m
./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing oddeven  -pir 0.008 poisson -seed 10   -verbose -1  -traffic transpose2 -sel dp > results/moe_0_trans2_008.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing dw_xy  -pir 0.002  poisson -seed 10 -verbose -1 -traffic transpose2  -bwt 80 > results/dw1_0_trans2_002.m
./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing dw_xy  -pir 0.004  poisson -seed 10 -verbose -1 -traffic transpose2  -bwt 80 > results/dw1_0_trans2_004.m
./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing dw_xy  -pir 0.006  poisson -seed 10 -verbose -1 -traffic transpose2  -bwt 80 > results/dw1_0_trans2_006.m
./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 100  -routing dw_xy  -pir 0.008  poisson -seed 10 -verbose -1 -traffic transpose2  -bwt 80 > results/dw1_0_trans2_008.m
