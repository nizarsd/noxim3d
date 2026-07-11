./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 200  -routing oddeven  -pir 0.01  poisson -seed 10 -verbose -1 -traffic random  -sel dp > results/oe2_dp_0_rand.m
./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 200  -routing oddeven  -pir 0.01  poisson -seed 10 -verbose -1 -traffic transpose2  -sel dp > results/oe2_dp_0_tans2.m
./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 200  -routing oddeven  -pir 0.01  poisson -seed 10 -verbose -1 -hs 122 0.05 -hs 123 0.05 -hs 128 0.05 -hs 129 0.05  -sel dp > results/oe2_dp_0_hs.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 200  -routing negativefirst  -pir 0.01  poisson -seed 10 -verbose -1 -traffic random  -sel dp > results/nf_dp_0_rand.m
./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 200  -routing negativefirst  -pir 0.01  poisson -seed 10 -verbose -1 -traffic transpose2  -sel dp > results/nf_dp_0_tans2.m
./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 200  -routing negativefirst  -pir 0.01  poisson -seed 10 -verbose -1 -hs 122 0.05 -hs 123 0.05 -hs 128 0.05 -hs 129 0.05  -sel dp > results/nf_dp_0_hs.m


./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 200  -routing dwoddeven  -pir 0.01  poisson -seed 10 -verbose -1 -traffic random  -sel dp -tmode 1  -tupper 90 -tlower 90 > results/dwoe1_90_rand.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 200  -routing dwoddeven  -pir 0.01  poisson -seed 10 -verbose -1 -traffic random  -sel dp -tmode 2  -tupper 90 -tlower 90 > results/dwoe2_90_rand.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 200  -routing oddeven  -pir 0.01  poisson -seed 10 -verbose -1 -traffic random  -sel dp -tmode 2  -tupper 90 -tlower 90 > results/oe2_90_rand.m


./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 200  -routing dwoddeven  -pir 0.01  poisson -seed 10 -verbose -1 -traffic transpose2  -sel dp -tmode 1  -tupper 90 -tlower 90 > results/dwoe1_90_trans.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 200  -routing dwoddeven  -pir 0.01  poisson -seed 10 -verbose -1 -traffic transpose2  -sel dp -tmode 2  -tupper 90 -tlower 90 > results/dwoe2_90_trans.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 200  -routing oddeven  -pir 0.01  poisson -seed 10 -verbose -1 -traffic transpose2  -sel dp -tmode 2  -tupper 90 -tlower 90 > results/oe2_90_trans.m


./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 200  -routing dwoddeven  -pir 0.01  poisson -seed 10 -verbose -1 -traffic transpose1  -sel dp -tmode 1  -tupper 90 -tlower 90 > results/dwoe1_90_trans1.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 200  -routing dwoddeven  -pir 0.01  poisson -seed 10 -verbose -1 -traffic transpose1  -sel dp -tmode 2  -tupper 90 -tlower 90 > results/dwoe2_90_trans1.m

./noxim -dimx 6  -dimy 6 -dimz 4   -sim 10000 -samp 200  -routing oddeven  -pir 0.01  poisson -seed 10 -verbose -1 -traffic transpose1  -sel dp -tmode 2  -tupper 90 -tlower 90 > results/oe2_90_trans1.m





