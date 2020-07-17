(defun notes ()
  (loop
     repeat 24
     for tim from 0 by 1/3
     do (fms:note :time tim :dur 1/3
                  :pitch (+ 60 (random 13))
                  :tupletdur (if (< tim 4) 1 2))))

(fms:with-score (:filename *filename*)
  (notes))
