#!/bin/sh

while [ 1 ]; do 
    ./pick_star.rb
    ./ave_frames 50 \!center.fits
    ./find_boxes.py center.fits
    cat center.fits | xpaset timDIMM fits
    ./gto900_log.rb >> gto900.log
    ./measure_seeing 10000 `tail -1 gto900.log | cut -d ' ' -f 8`
    ./gto900_guide.rb
    mv centroids.dat data/centroids_`date -u '+%Y%m%d-%H%M%S'`.dat
    ./plot.gnu
    echo "image;text 25 5 # text={Seeing = `cat seeing.out`\"}" | xpaset timDIMM regions
    echo "image;text 290 5 # text={R0 = `cat r0.out` cm}" | xpaset timDIMM regions
    scp seeing.out seeing.dat massdimm@massdimm:~/seeing/suthdimm/.
    xpaset -p timDIMM saveimage png ds9.png
    mv ds9.png /Users/timdimm/Sites/images/.
done
