(defun notes (n r)
  (loop
     repeat n
     for tim from 0 by r
     do (fms:note :time tim :dur r :pitch 60
                  :staff (if (< (random 1.0) 0.5) 1 2))))

(fms:with-score (:filename *filename*)
  (notes 16 1/2))
