# Bhaskar Rao Designs generated by CPro1
The parameters for each design are given in the filename, for example result-15-126-42-5-12-seed1000.txt has parameters (v,b,r,k,L) = (15,126,42,5,12).

The code progBRD1.c and progBRD2.c were generated by CPro1 using o3-mini-high as the LLM, and were used to construct these designs, as follows:
progBRD1.c, with hyperparameters selected by hyperparameter tuning: [0.6666666666666666, 3.3333333333333335]
result-15-42-14-5-4-seed1000.txt
result-16-48-15-5-4-seed1000.txt

progBRD2.c,with hyperparameters selected by hyperparameter tuning: [100.0, 0.9, 0.2236079157811727, 1.0]
result-15-126-42-5-12-seed1000.txt
(Note progBRD2.c uses the "2-phase" approach mentioned in the paper.)

The commandline parameters are: instance-parameters seed hyperparameters

For example, after compiling progBRD2.c to progBRD2, result-15-126-42-5-12-seed1000.txt was produced by:

progBRD2 15 126 42 5 12 1000 100.0 0.9 0.2236079157811727 1.0

The runs that generated these took up to 48 hours on a AMD Ryzen 9 7950X3D CPU.

If you want to exactly reproduce the results of these runs, you'll generally need a compatible implementation of the random number generator.  These results were run under Ubuntu 24.4 with gcc 13.3.0 using glibc 2.39, compiled as follows:

gcc -march=native -O3 -o progBRD2 progBRD2.c -lm

Similar versions of gcc under Linux are likely to give similar results, but note that MacOS by default uses a different library with a different random number generator and won't give the same results.
