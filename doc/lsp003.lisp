(fms:with-score
    (:filename *filename*
     :sets '(:quartertones t)
     :parts '((:id pno :inst :piano)))
  (loop
     repeat 21
     for tim from 0 by 1/2
     do (fms:note :time tim
                  :dur (if (< tim 10) 1/2 1)
                  :pitch (+ 70 (/ (random 4) 2)))))
