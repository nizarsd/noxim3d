./noxim -dimx 7  -dimy 7 -dimz 1 -sim 10000 -samp 100  -tmode 0  -seed 10 -routing negativefirst -size 8 8 -traffic table real_tr/traffic_ami49_f.txt > real_tr/rand0_nf_xx_ami49_f.m

./noxim -dimx 7  -dimy 7 -dimz 1 -sim 10000 -samp 100  -tmode 2  -seed 10 -routing oddeven -sel dp -size 8 8 -traffic table real_tr/traffic_ami49_f.txt -tupper 55 -tlower 55 > real_tr/dp2_oe_55_ami49_f.m

./noxim -dimx 7  -dimy 7 -dimz 1 -sim 10000 -samp 100  -tmode 2  -seed 10 -routing negativefirst -sel dp -size 8 8 -traffic table real_tr/traffic_ami49_f.txt  -tupper 55 -tlower 55 > real_tr/dp2_nf_55_ami49_f.m

