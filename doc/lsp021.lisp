(defun notes ()
  (loop
     for i from 0 below 24
     for tim from 0 by 1/3
     for m in '#1=(("tup..") () () . #1#)
     do (fms:note :time tim :dur 1/3
                  :pitch (+ 60 (random 13))
                  :marks m)))

(fms:with-score (:filename *filename*)
  (notes))
