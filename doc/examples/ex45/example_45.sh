#!/bin/bash
#               GMT EXAMPLE 44
#               $Id$
#
# Purpose:      Illustrate use of map inserts
# GMT progs:    pscoast, psbasemap, mapproject
# Unix progs:   read
#

ps=example_45.ps
# Basic LS line y = a + bx
gmt trend1d -Fxm CO2.txt -Np1 > model.txt
gmt psxy -R1958/2016/310/410 -JX6i/1.9i -P -Bxaf -Byaf+u" ppm" -BWSne+gazure1 -Sc0.05c -Gred -K CO2.txt -X1.5i > $ps
gmt psxy -R -J -O -K -W0.5p,blue model.txt >> $ps
# Basic LS line y = a + bx + cx^2
gmt trend1d -Fxm CO2.txt -Np2 > model.txt
gmt psxy -R -J -O -Bxaf -Byaf+u" ppm" -BWSne+gazure1 -Sc0.05c -Gred -K CO2.txt -Y2.3i >> $ps
gmt psxy -R -J -O -K -W0.5p,blue model.txt >> $ps
# Basic LS line y = a + bx + cx^2 + seasonal change
gmt trend1d -Fxmr CO2.txt -Np2,f1+o1958+l1 > model.txt
gmt psxy -R -J -O -Bxaf -Byaf+u" ppm" -BWSne+gazure1 -Sc0.05c -Gred -K CO2.txt -Y2.3i >> $ps
gmt psxy -R -J -O -K -W0.25p,blue model.txt >> $ps
# Plot residuals of last model
gmt psxy -R1958/2016/-4/4 -J -O -Bxaf -Byafg10+u" ppm" -BWSne+t"The Keeling Curve [CO@-2@- at Mauna Loa]"+gazure1 -Sc0.05c -Gred -K model.txt -i0,2 -Y2.3i >> $ps
gmt psxy -R -J -O -T >> $ps
rm -f model.txt
