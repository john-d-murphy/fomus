(fms:with-score
    (:filename *filename*
     :sets '((:title "Random")
             (:author "David Psenicka")
             (:quartertones t)))
  (loop
     for o from 0 below 20 by 1/2
     do (fms:note :time o :dur 1/2 :pitch (+ 65 (random 15.0)))))
