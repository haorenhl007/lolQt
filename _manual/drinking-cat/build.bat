..\mencoder.exe mf://@files.txt -mf w=320:h=240:fps=18.15:type=png -ovc lavc -lavcopts vcodec=mpeg4:mbd=2:trell:aspect=4/3 -oac mp3lame -lameopts cbr:br=160 -audiofile "Dire Straits - 02 - Water of Love.mp3" -sub sub.srt -subfont-text-scale 3 -o cat7.avi