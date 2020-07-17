(fms:setting :filename *filename*)
(fms:setting :title "Random")
(fms:setting :author "David Psenicka")
(fms:setting :quartertones t)

(loop
   for o from 0 below 20 by 1/2
   do (fms:note :time o :dur 1/2 :pitch (+ 65 (random 15.0))))

(fms:run)
