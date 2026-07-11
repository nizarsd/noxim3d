./noxim -dimx 8  -dimy 10 -dimz 4 -sim 10000 -samp 100  -pir 0.01 poisson -tmode 1  -tlower 85  -tupper 90  -bwt 1000 -cinterval 500 -routing dw_xyz -size 8 8 -seed 10 >>result1.m
 
./noxim -dimx 8  -dimy 10 -dimz 4 -sim 10000 -samp 100  -pir 0.01 poisson -tmode 1  -tlower 85  -tupper 90  -bwt 1000 -cinterval 500 -routing xyz -size 8 8 -seed 10 -seed 10 >>result2.m

./noxim -dimx 8  -dimy 10 -dimz 4 -sim 10000 -samp 100  -pir 0.01 poisson -bwt 1000 -cinterval 500 -routing dw_xyz -size 8 8 -seed 10 >>result3.m

./noxim -dimx 8  -dimy 10 -dimz 4 -sim 10000 -samp 100  -pir 0.01 poisson -routing xyz -size 8 8 -seed 10  >>result4.m
